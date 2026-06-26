#!/usr/bin/env python3
# vantage_gitstar_tui.py: Terminal-based UI for GitHub starred repositories analyzer
# Provides an intuitive interface to the VANTAGE GitStar functionality

# Standard library imports
import os
import sys
import threading
import time
import signal
from pathlib import Path
import importlib.util

# Third-party imports (with robust error handling)
try:
    import npyscreen
    DEPENDENCIES_MET = True
except ImportError as e:
    print(f"Missing dependency: {e}")
    print("Install with: pip install npyscreen tqdm")
    DEPENDENCIES_MET = False

# Try to import the GitHubStarAnalyzer from vantage_gitstar
ANALYZER_AVAILABLE = False
GITSTAR_PATH = os.path.join(os.path.dirname(os.path.abspath(__file__)), "vantage_gitstar.py")

if os.path.exists(GITSTAR_PATH):
    try:
        # Dynamic import of the analyzer module
        spec = importlib.util.spec_from_file_location("vantage_gitstar", GITSTAR_PATH)
        gitstar_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(gitstar_module)
        GitHubStarAnalyzer = gitstar_module.GitHubStarAnalyzer
        ANALYZER_AVAILABLE = True
    except Exception as e:
        print(f"Error loading GitHubStarAnalyzer: {e}")
        ANALYZER_AVAILABLE = False

# Constants
HOME_DIR = os.path.expanduser("~")
GITSTAR_DIR = os.path.join(HOME_DIR, ".vantage", "gitstar")

# Ensure directories exist
Path(GITSTAR_DIR).mkdir(parents=True, exist_ok=True)

# Global analyzer instance
analyzer = None
if ANALYZER_AVAILABLE:
    analyzer = GitHubStarAnalyzer()

# Background operation class


class BackgroundOperation(threading.Thread):
    def __init__(self, operation, args=None, on_complete=None):
        threading.Thread.__init__(self)
        self.daemon = True
        self.operation = operation
        self.args = args or []
        self.on_complete = on_complete
        self.result = None
        self.is_running = False
        self.progress = 0
        self.status = "Idle"

    def run(self):
        self.is_running = True
        self.status = "Running..."
        try:
            self.result = self.operation(*self.args)
            self.status = "Completed"
        except Exception as e:
            self.status = f"Error: {str(e)}"
            self.result = None
        finally:
            self.is_running = False
            if self.on_complete:
                self.on_complete(self.result)

# Main form class for the GitStar TUI


class GitStarTUI(npyscreen.NPSAppManaged):
    def onStart(self):
        self.addForm("MAIN", MainMenu, name="VANTAGE GitHub Star Analyzer")
        self.addForm("FETCH", FetchReposForm, name="Fetch GitHub Repositories")
        self.addForm("ANALYZE", AnalyzeReposForm, name="Analyze Repositories")
        self.addForm("SEARCH", SearchReposForm, name="Search Repositories")
        self.addForm("SUGGEST", SuggestReposForm, name="Get Repository Suggestions")
        self.addForm("STATS", StatsForm, name="Repository Statistics")
        self.addForm("DETAILS", RepoDetailsForm, name="Repository Details")

# Main menu form


class MainMenu(npyscreen.FormBaseNew):
    def create(self):
        self.add(npyscreen.TitleText, name="VANTAGE GitHub Star Analyzer", editable=False)
        self.add(npyscreen.MultiLine, max_height=2, editable=False,
                 value=["Terminal UI for managing and analyzing GitHub starred repositories.",
                        "Select an option below:"])

        self.add(npyscreen.Textfield, editable=False, value="")

        # Main menu options
        self.menu = self.add(npyscreen.MultiLine,
                             values=["1. Fetch Repositories from GitHub",
                                     "2. Analyze Downloaded Repositories",
                                     "3. Search Repositories",
                                     "4. Get Repository Suggestions",
                                     "5. View Repository Statistics",
                                     "6. Exit"],
                             max_height=8,
                             scroll_exit=True)

        self.add(npyscreen.Textfield, editable=False, value="")

        # Status information
        stats = {"total_repos": 0, "readmes_downloaded": 0, "analyzed_repos": 0}
        if ANALYZER_AVAILABLE and analyzer:
            stats = analyzer.get_statistics()

        status_text = f"Status: {stats['total_repos']} repositories | {stats['readmes_downloaded']} READMEs | {stats['analyzed_repos']} analyzed"
        self.status = self.add(npyscreen.TitleText, name=status_text, editable=False)

        # Dependency check
        if not DEPENDENCIES_MET:
            self.add(npyscreen.MultiLine, max_height=2, editable=False,
                     value=["Warning: Missing dependencies. Install with:",
                            "pip install npyscreen tqdm requests beautifulsoup4 numpy scipy scikit-learn"])

        # Key bindings info
        self.add(npyscreen.FixedText, value="Press Enter to select, q to exit", editable=False, rely=-3)

    def afterEditing(self):
        selected = self.menu.value

        if selected == 0:  # Fetch repositories
            self.parentApp.switchForm("FETCH")
        elif selected == 1:  # Analyze repositories
            self.parentApp.switchForm("ANALYZE")
        elif selected == 2:  # Search repositories
            self.parentApp.switchForm("SEARCH")
        elif selected == 3:  # Get suggestions
            self.parentApp.switchForm("SUGGEST")
        elif selected == 4:  # View statistics
            self.parentApp.switchForm("STATS")
        elif selected == 5:  # Exit
            self.parentApp.setNextForm(None)

# Form for fetching repositories


class FetchReposForm(npyscreen.ActionForm):
    def create(self):
        self.username = self.add(npyscreen.TitleText, name="GitHub Username:", value="")
        self.add(npyscreen.FixedText, value="", editable=False)
        self.password = self.add(npyscreen.TitlePassword, name="Password (optional):", value="")
        self.add(npyscreen.FixedText, value="", editable=False)

        self.status = self.add(npyscreen.TitleText, name="Status:", value="Ready", editable=False)
        self.progress = self.add(npyscreen.TitleSlider, name="Progress:", value=0,
                                 out_of=100, step=1, editable=False)

        self.operation = None

    def on_ok(self):
        if not ANALYZER_AVAILABLE:
            npyscreen.notify_confirm("GitHubStarAnalyzer not available. Please check dependencies.", title="Error")
            return

        username = self.username.value.strip()
        if not username:
            npyscreen.notify_confirm("Please enter a GitHub username.", title="Error")
            return

        password = self.password.value.strip() or None

        # Start background operation
        def fetch_complete(result):
            if result:
                self.status.value = "Fetch completed successfully!"
                self.progress.value = 100
            else:
                self.status.value = "Fetch failed. See console for details."
            self.display()

        # Define update function for progress display
        def update_progress():
            i = 0
            while self.operation and self.operation.is_running:
                time.sleep(0.1)
                i = (i + 1) % 90
                self.progress.value = 10 + i  # 10-99% during processing
                self.status.value = f"Fetching repositories for {username}..."
                self.display()

        # Set initial status
        self.status.value = "Starting fetch operation..."
        self.progress.value = 5
        self.display()

        # Start operation in background
        self.operation = BackgroundOperation(
            analyzer.fetch_starred_repos,
            args=[username, password],
            on_complete=fetch_complete
        )
        self.operation.start()

        # Start progress update thread
        progress_thread = threading.Thread(target=update_progress)
        progress_thread.daemon = True
        progress_thread.start()

    def on_cancel(self):
        self.parentApp.switchFormPrevious()

# Form for analyzing repositories


class AnalyzeReposForm(npyscreen.ActionForm):
    def create(self):
        stats = {"total_repos": 0, "readmes_downloaded": 0, "analyzed_repos": 0}
        if ANALYZER_AVAILABLE and analyzer:
            stats = analyzer.get_statistics()

        self.add(npyscreen.TitleText, name="Repository Analysis", editable=False)
        self.add(npyscreen.FixedText, value="", editable=False)
        self.add(npyscreen.TitleText, name="Available READMEs:",
                 value=f"{stats['readmes_downloaded']} of {stats['total_repos']} repositories",
                 editable=False)
        self.add(npyscreen.FixedText, value="", editable=False)

        self.status = self.add(npyscreen.TitleText, name="Status:", value="Ready", editable=False)
        self.progress = self.add(npyscreen.TitleSlider, name="Progress:", value=0,
                                 out_of=100, step=1, editable=False)

        self.operation = None

    def on_ok(self):
        if not ANALYZER_AVAILABLE:
            npyscreen.notify_confirm("GitHubStarAnalyzer not available. Please check dependencies.", title="Error")
            return

        # Start background operation
        def analyze_complete(result):
            if result:
                self.status.value = "Analysis completed successfully!"
                self.progress.value = 100
            else:
                self.status.value = "Analysis failed. See console for details."
            self.display()

        # Define update function for progress display
        def update_progress():
            i = 0
            while self.operation and self.operation.is_running:
                time.sleep(0.1)
                i = (i + 1) % 90
                self.progress.value = 10 + i  # 10-99% during processing
                self.status.value = "Analyzing repositories..."
                self.display()

        # Set initial status
        self.status.value = "Starting analysis operation..."
        self.progress.value = 5
        self.display()

        # Start operation in background
        self.operation = BackgroundOperation(
            analyzer.analyze_readmes,
            on_complete=analyze_complete
        )
        self.operation.start()

        # Start progress update thread
        progress_thread = threading.Thread(target=update_progress)
        progress_thread.daemon = True
        progress_thread.start()

    def on_cancel(self):
        self.parentApp.switchFormPrevious()

# Form for searching repositories


class SearchReposForm(npyscreen.ActionForm):
    def create(self):
        self.add(npyscreen.TitleText, name="Repository Search", editable=False)
        self.add(npyscreen.FixedText, value="", editable=False)

        self.query = self.add(npyscreen.TitleText, name="Search Query:", value="")
        self.add(npyscreen.FixedText, value="", editable=False)

        self.status = self.add(npyscreen.TitleText, name="Status:", value="Enter search terms", editable=False)

        # Results list
        self.results_label = self.add(npyscreen.TitleText, name="Results:", value="", editable=False)
        self.results = self.add(npyscreen.MultiLine, values=[], max_height=10, scroll_exit=True)

        self.operation = None
        self.search_results = []

    def on_ok(self):
        if not ANALYZER_AVAILABLE:
            npyscreen.notify_confirm("GitHubStarAnalyzer not available. Please check dependencies.", title="Error")
            return

        query = self.query.value.strip()
        if not query:
            npyscreen.notify_confirm("Please enter a search query.", title="Error")
            return

        # Start background operation
        def search_complete(results):
            self.search_results = results or []
            result_values = []

            if not results or len(results) == 0:
                self.status.value = "No matching repositories found."
                self.results_label.value = "Results: None"
            else:
                self.status.value = f"Found {len(results)} matching repositories."
                self.results_label.value = f"Results: {len(results)} repositories"

                # Format results for display
                for i, repo in enumerate(results[:30]):  # Limit to 30 for display
                    desc = repo.get('description', 'No description') or 'No description'
                    if len(desc) > 60:
                        desc = desc[:57] + "..."
                    result_values.append(f"{i+1}. {repo['full_name']} - {desc}")

            # Update the results list
            self.results.values = result_values
            self.results.value = 0  # Select first item
            self.display()

        # Set initial status
        self.status.value = f"Searching for '{query}'..."
        self.results.values = []
        self.display()

        # Start operation in background
        self.operation = BackgroundOperation(
            analyzer.search_repositories,
            args=[query],
            on_complete=search_complete
        )
        self.operation.start()

    def on_cancel(self):
        self.parentApp.switchFormPrevious()

    def h_exit_down(self, *args, **kwargs):
        # Handle selection of a repository from the results list
        if hasattr(self, 'results') and hasattr(self, 'search_results'):
            if self.results.value is not None and self.search_results and len(self.search_results) > self.results.value:
                # Store the selected repository for the details form
                self.parentApp.getForm("DETAILS").repository = self.search_results[self.results.value]
                self.parentApp.switchForm("DETAILS")

# Form for repository suggestions


class SuggestReposForm(npyscreen.ActionForm):
    def create(self):
        self.add(npyscreen.TitleText, name="Repository Suggestions", editable=False)
        self.add(npyscreen.FixedText, value="", editable=False)

        self.task = self.add(npyscreen.TitleText, name="Task Description:", value="")
        self.add(npyscreen.FixedText, value="Describe what you need to accomplish", editable=False)
        self.add(npyscreen.FixedText, value="", editable=False)

        self.status = self.add(npyscreen.TitleText, name="Status:", value="Enter task description", editable=False)

        # Results list
        self.results_label = self.add(npyscreen.TitleText, name="Suggestions:", value="", editable=False)
        self.results = self.add(npyscreen.MultiLine, values=[], max_height=10, scroll_exit=True)

        self.operation = None
        self.suggestion_results = []

    def on_ok(self):
        if not ANALYZER_AVAILABLE:
            npyscreen.notify_confirm("GitHubStarAnalyzer not available. Please check dependencies.", title="Error")
            return

        task = self.task.value.strip()
        if not task:
            npyscreen.notify_confirm("Please enter a task description.", title="Error")
            return

        # Start background operation
        def suggest_complete(results):
            self.suggestion_results = [s["repository"] for s in results] if results else []
            result_values = []

            if not results or len(results) == 0:
                self.status.value = "No suitable repositories found."
                self.results_label.value = "Suggestions: None"
            else:
                self.status.value = f"Found {len(results)} suggested repositories."
                self.results_label.value = f"Suggestions: {len(results)} repositories"

                # Format results for display
                for i, suggestion in enumerate(results[:30]):  # Limit to 30 for display
                    repo = suggestion["repository"]
                    reason = suggestion["reason"]
                    result_values.append(f"{i+1}. {repo['full_name']} - {reason[:50]}")

            # Update the results list
            self.results.values = result_values
            self.results.value = 0  # Select first item
            self.display()

        # Set initial status
        self.status.value = f"Finding repositories for '{task}'..."
        self.results.values = []
        self.display()

        # Start operation in background
        self.operation = BackgroundOperation(
            analyzer.suggest_repositories,
            args=[task],
            on_complete=suggest_complete
        )
        self.operation.start()

    def on_cancel(self):
        self.parentApp.switchFormPrevious()

    def h_exit_down(self, *args, **kwargs):
        # Handle selection of a repository from the results list
        if hasattr(self, 'results') and hasattr(self, 'suggestion_results'):
            if self.results.value is not None and self.suggestion_results and len(
                    self.suggestion_results) > self.results.value:
                # Store the selected repository for the details form
                self.parentApp.getForm("DETAILS").repository = self.suggestion_results[self.results.value]
                self.parentApp.switchForm("DETAILS")

# Form for repository statistics


class StatsForm(npyscreen.Form):
    def create(self):
        stats = {"total_repos": 0, "readmes_downloaded": 0, "analyzed_repos": 0,
                 "llm_analyzed": 0, "languages": {}, "clusters": {}, "last_updated": None}

        if ANALYZER_AVAILABLE and analyzer:
            stats = analyzer.get_statistics()

        self.add(npyscreen.TitleText, name="Repository Statistics", editable=False)
        self.add(npyscreen.FixedText, value="", editable=False)

        # Basic stats
        self.add(npyscreen.TitleText, name="Total Repositories:",
                 value=str(stats["total_repos"]), editable=False)
        self.add(npyscreen.TitleText, name="READMEs Downloaded:",
                 value=str(stats["readmes_downloaded"]), editable=False)
        self.add(npyscreen.TitleText, name="Analyzed Repositories:",
                 value=str(stats["analyzed_repos"]), editable=False)
        self.add(npyscreen.TitleText, name="LLM-Analyzed Repositories:",
                 value=str(stats["llm_analyzed"]), editable=False)

        # Last updated
        if stats["last_updated"]:
            self.add(npyscreen.TitleText, name="Last Updated:",
                     value=stats["last_updated"], editable=False)

        self.add(npyscreen.FixedText, value="", editable=False)

        # Languages section
        self.add(npyscreen.TitleText, name="Top Languages:", value="", editable=False)
        lang_values = []
        for lang, count in list(stats["languages"].items())[:10]:  # Top 10 languages
            lang_values.append(f"{lang}: {count}")

        self.languages = self.add(npyscreen.MultiLine, values=lang_values,
                                  max_height=min(len(lang_values) + 1, 5), editable=False)

        self.add(npyscreen.FixedText, value="", editable=False)

        # Clusters section
        if stats["clusters"]:
            self.add(npyscreen.TitleText, name="Repository Clusters:", value="", editable=False)
            cluster_values = []
            for cluster_id, data in stats["clusters"].items():
                cluster_values.append(f"{data['name']}: {data['count']} repositories")

            self.clusters = self.add(npyscreen.MultiLine, values=cluster_values,
                                     max_height=min(len(cluster_values) + 1, 5), editable=False)

        self.add(npyscreen.FixedText, value="Press any key to return to main menu", rely=-3, editable=False)

    def afterEditing(self):
        self.parentApp.switchFormPrevious()

# Form for repository details


class RepoDetailsForm(npyscreen.Form):
    def create(self):
        self.repository = None
        self.name = self.add(npyscreen.TitleText, name="Repository:", value="", editable=False)
        self.url = self.add(npyscreen.TitleText, name="URL:", value="", editable=False)
        self.description = self.add(npyscreen.TitleText, name="Description:", value="", editable=False)
        self.language = self.add(npyscreen.TitleText, name="Language:", value="", editable=False)
        self.stars = self.add(npyscreen.TitleText, name="Stars:", value="", editable=False)
        self.forks = self.add(npyscreen.TitleText, name="Forks:", value="", editable=False)

        self.add(npyscreen.FixedText, value="", editable=False)

        # LLM analysis section (if available)
        self.analysis_section = self.add(npyscreen.TitleText, name="LLM Analysis:", value="", editable=False)
        self.purpose = self.add(npyscreen.TitleText, name="Purpose:", value="", editable=False)
        self.category = self.add(npyscreen.TitleText, name="Category:", value="", editable=False)

        self.add(npyscreen.FixedText, value="", editable=False)

        # Features section
        self.features_title = self.add(npyscreen.TitleText, name="Features:", value="", editable=False)
        self.features = self.add(npyscreen.MultiLine, values=[], max_height=5, editable=False)

        self.add(npyscreen.FixedText, value="Press any key to return", rely=-3, editable=False)

    def beforeEditing(self):
        # Update form with repository details
        if self.repository:
            self.name.value = self.repository.get("full_name", "Unknown")
            self.url.value = self.repository.get("html_url", "Unknown")
            self.description.value = self.repository.get("description", "No description") or "No description"
            self.language.value = self.repository.get("language", "Unknown") or "Unknown"
            self.stars.value = str(self.repository.get("stars", 0))
            self.forks.value = str(self.repository.get("forks", 0))

            # Check for LLM analysis
            if "llm_analysis" in self.repository:
                analysis = self.repository["llm_analysis"]
                self.purpose.value = analysis.get("purpose", "Unknown")
                self.category.value = analysis.get("category", "Unknown")

                # Features
                features = analysis.get("features", [])
                if features:
                    self.features.values = features
                else:
                    self.features.values = ["No features found in analysis"]
            else:
                self.purpose.value = "Not available (run LLM analysis)"
                self.category.value = "Not available (run LLM analysis)"
                self.features.values = ["No LLM analysis available"]

    def afterEditing(self):
        self.parentApp.switchFormPrevious()

# Main function


def main():
    if not DEPENDENCIES_MET:
        print("Warning: Missing Python dependencies.")
        print("Install with: pip install npyscreen tqdm")
        print("Attempting to continue...")

    if not ANALYZER_AVAILABLE:
        print("Error: GitHubStarAnalyzer not available.")
        print(f"Expected to find it at: {GITSTAR_PATH}")
        return 1

    # Handle keyboard interrupts properly
    signal.signal(signal.SIGINT, signal.SIG_DFL)

    # Start the application
    app = GitStarTUI()
    try:
        app.run()
    except KeyboardInterrupt:
        print("Exiting...")

    return 0


if __name__ == "__main__":
    sys.exit(main())
