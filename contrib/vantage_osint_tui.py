#!/usr/bin/env python3
# VANTAGE OSINT TUI - Terminal User Interface for OSINT Tool Management
# Provides a user-friendly interface for managing OSINT tools and workflows

# Standard library imports
import os
import sys
import time
import threading
from datetime import datetime
import curses

# Third-party imports (with robust error handling)
try:
    import npyscreen
except ImportError as e:
    print(f"Missing dependency: {e}")
    print("Install with: pip install npyscreen tqdm")
    sys.exit(1)

# Ensure access to vantage_osint functions
script_dir = os.path.dirname(os.path.abspath(__file__))
if script_dir not in sys.path:
    sys.path.append(script_dir)

try:
    from vantage_osint import (
        load_tool_database, find_tools_by_data_type, suggest_tools,
        generate_workflow, parse_usage_log, calculate_stats,
        build_database
    )
except ImportError:
    npyscreen.notify_confirm("Could not import OSINT module functions. Please ensure vantage_osint.py is available.",
                             title="Import Error")
    sys.exit(1)

# Check for LLM availability
LLM_AVAILABLE = False
try:
    pass  # LLM import removed as unused
    LLM_AVAILABLE = False
except ImportError:
    pass

# Define color pairs


def setup_colors():
    curses.init_pair(1, curses.COLOR_WHITE, curses.COLOR_BLUE)  # Title bar
    curses.init_pair(2, curses.COLOR_BLACK, curses.COLOR_GREEN)  # Status bar
    curses.init_pair(3, curses.COLOR_YELLOW, curses.COLOR_BLACK)  # Highlights
    curses.init_pair(4, curses.COLOR_CYAN, curses.COLOR_BLACK)  # Category titles
    curses.init_pair(5, curses.COLOR_GREEN, curses.COLOR_BLACK)  # Success
    curses.init_pair(6, curses.COLOR_RED, curses.COLOR_BLACK)  # Error/Warning
    curses.init_pair(7, curses.COLOR_WHITE, curses.COLOR_BLACK)  # Normal text
    curses.init_pair(8, curses.COLOR_BLACK, curses.COLOR_CYAN)  # Selected item
    curses.init_pair(9, curses.COLOR_MAGENTA, curses.COLOR_BLACK)  # Secondary highlight

# Custom BoxTitle widget with color support


class ColorBoxTitle(npyscreen.BoxTitle):
    def __init__(self, *args, **keywords):
        super(ColorBoxTitle, self).__init__(*args, **keywords)
        self.color = "LABEL"  # Default color

    def update(self, clear=True):
        if clear:
            self.clear()
        if self.hidden:
            self.clear()
            return False

        # Draw the box and title
        self.box()
        if self.name:
            name_attributes = self.parent.theme_manager.findPair(self, self.color)
            self.parent.curses_pad.addstr(self.rely, self.relx + 2, self.name, name_attributes)

        # Let contained objects draw themselves
        for w in self._my_widgets:
            w.update(clear=clear)

# Custom MultiLine widget with colored entries


class ColoredMultiLine(npyscreen.MultiLine):
    def _print_line(self, line, value_indexer):
        line_color = self.parent.theme_manager.findPair(self, self.color)

        # Override color for certain values
        if hasattr(value_indexer, 'color'):
            line_color = self.parent.theme_manager.findPair(self, value_indexer.color)
        elif hasattr(value_indexer, 'category') and value_indexer.category == 'reconnaissance':
            line_color = curses.color_pair(3) | curses.A_BOLD  # Yellow for recon tools
        elif hasattr(value_indexer, 'category') and value_indexer.category == 'social_media':
            line_color = curses.color_pair(4) | curses.A_BOLD  # Cyan for social media tools
        elif hasattr(value_indexer, 'category') and value_indexer.category == 'password_cracking':
            line_color = curses.color_pair(6) | curses.A_BOLD  # Red for password tools

        # Print selected line with highlight
        if self.value and line == self.value:
            line_color = curses.color_pair(8) | curses.A_BOLD

        # Actually print the line
        self.parent.curses_pad.addstr(self.rely + line, self.relx, str(value_indexer)[:self.width - 2], line_color)

# Custom button with background color


class ColorButton(npyscreen.ButtonPress):
    def __init__(self, *args, **keywords):
        self.bg_color = keywords.pop('bg_color', None)
        super(ColorButton, self).__init__(*args, **keywords)

    def update(self, clear=True):
        if clear:
            self.clear()
        if self.hidden:
            self.clear()
            return False

        # Custom color
        if self.bg_color:
            attributes = curses.color_pair(self.bg_color)
            if self.do_colors():
                if self.editing:
                    attributes = curses.color_pair(self.bg_color) | curses.A_BOLD
                elif self.highlight:
                    attributes = curses.color_pair(self.bg_color) | curses.A_BOLD
            self.parent.curses_pad.addstr(self.rely, self.relx, self.name, attributes)
        else:
            # Default behavior
            attributes = self.parent.theme_manager.findPair(self)
            if self.do_colors():
                if self.editing:
                    attributes = self.parent.theme_manager.findPair(self, 'CONTROL')
                elif self.highlight:
                    attributes = self.parent.theme_manager.findPair(self, 'CONTROL')
            self.parent.curses_pad.addstr(self.rely, self.relx, self.name, attributes)

# Custom titled box with fixed height


class FixedHeightBoxTitle(ColorBoxTitle):
    def resize(self):
        super(FixedHeightBoxTitle, self).resize()
        self.height = self.max_height  # Keep height fixed at max_height

# OSINT Tool class to represent a tool in the database


class OSINTTool:
    def __init__(self, name, info):
        self.name = name
        self.description = info.get('description', 'No description available')
        self.category = info.get('category', 'unknown')
        self.data_types = info.get('data_types', [])
        self.url = info.get('url', '')
        self.stars = info.get('stars', 0)
        self.source = info.get('source', 'unknown')

    def __str__(self):
        return f"{self.name} [{self.category}]"

# Helper class for workflow steps


class WorkflowStep:
    def __init__(self, step_num, tool, purpose, note=None):
        self.step_num = step_num
        self.tool = tool
        self.purpose = purpose
        self.note = note
        self.color = "DEFAULT"

        # Set color based on if tool is '?' or not
        if tool == "?":
            self.color = "DANGER"

    def __str__(self):
        result = f"Step {self.step_num}: {self.tool}"
        if self.note:
            result += f" ({self.note})"
        return result

# Form for displaying tool details


class ToolInfoForm(npyscreen.Form):
    def create(self):
        self.name = "Tool Information"

        # Add fields for tool details
        self.add(npyscreen.TitleText, name="Name:", value="", editable=False)
        self.add(npyscreen.TitleText, name="Category:", value="", editable=False)
        self.add(npyscreen.TitleMultiLine, name="Description:", values=[], max_height=4, editable=False)
        self.add(npyscreen.TitleText, name="Data Types:", value="", editable=False)
        self.add(npyscreen.TitleText, name="URL:", value="", editable=False)
        self.add(npyscreen.TitleText, name="Source:", value="", editable=False)

        # Add a button to close the form
        self.add(npyscreen.ButtonPress, name="Back", relx=35, rely=-3, when_pressed_function=self.exit_editing)

    def beforeEditing(self):
        # Get tool information from the parent form
        if hasattr(self.parentApp, 'selected_tool'):
            tool = self.parentApp.selected_tool
            # Update fields with tool information
            self.get_widget(0).value = tool.name
            self.get_widget(1).value = tool.category

            # Split long descriptions into multiple lines
            desc = tool.description
            desc_lines = []
            while desc:
                if len(desc) > 60:
                    pos = desc[:60].rfind(' ')
                    if pos == -1:
                        pos = 60
                    desc_lines.append(desc[:pos])
                    desc = desc[pos:].strip()
                else:
                    desc_lines.append(desc)
                    desc = ""

            self.get_widget(2).values = desc_lines
            self.get_widget(3).value = ", ".join(tool.data_types)
            self.get_widget(4).value = tool.url
            self.get_widget(5).value = tool.source

    def afterEditing(self):
        self.parentApp.switchFormPrevious()

# Form for displaying workflow information


class WorkflowInfoForm(npyscreen.Form):
    def create(self):
        self.name = "OSINT Workflow"

        # Add fields for workflow details
        self.add(npyscreen.TitleText, name="Investigation Type:", value="", editable=False)
        self.add(npyscreen.TitleMultiLine, name="Steps:", values=[], max_height=10, editable=False)

        # Add a note field
        self.add(npyscreen.TitleMultiLine, name="Note:", values=[], max_height=3, editable=False)

        # Add a button to close the form
        self.add(npyscreen.ButtonPress, name="Back", relx=35, rely=-3, when_pressed_function=self.exit_editing)

    def beforeEditing(self):
        # Get workflow information from the parent form
        if hasattr(self.parentApp, 'workflow_data'):
            workflow = self.parentApp.workflow_data
            # Update fields with workflow information
            self.get_widget(0).value = workflow.get('investigation_type', '')

            # Add steps to the list
            steps = []
            for step in workflow.get('workflow', []):
                step_text = f"{step.get('step', '?')}. Use {step.get('tool', '?')}"
                step_text += f"\n   Purpose: {step.get('purpose', '')}"
                if 'note' in step:
                    step_text += f"\n   Note: {step.get('note', '')}"
                steps.append(step_text)
                steps.append("")  # Add a blank line between steps

            self.get_widget(1).values = steps

            # Add note
            if 'note' in workflow:
                note_lines = []
                note = workflow['note']
                while note:
                    if len(note) > 60:
                        pos = note[:60].rfind(' ')
                        if pos == -1:
                            pos = 60
                        note_lines.append(note[:pos])
                        note = note[pos:].strip()
                    else:
                        note_lines.append(note)
                        note = ""
                self.get_widget(2).values = note_lines

    def afterEditing(self):
        self.parentApp.switchFormPrevious()

# Form for entering search queries


class SearchForm(npyscreen.Form):
    def create(self):
        self.name = "Search OSINT Tools"

        # Add field for search query
        self.add(npyscreen.TitleText, name="Search Query:", value="")

        # Add search type selector
        self.add(npyscreen.TitleSelectOne, name="Search Type:",
                 values=["Data Type Search", "Task Suggestion", "Workflow Generation"],
                 scroll_exit=True, max_height=4)

        # Add buttons
        self.add(npyscreen.ButtonPress, name="Search", relx=20, rely=10, when_pressed_function=self.on_search)
        self.add(npyscreen.ButtonPress, name="Cancel", relx=30, rely=10, when_pressed_function=self.exit_editing)

    def on_search(self):
        # Get search query and type
        query = self.get_widget(0).value
        search_type = self.get_widget(1).value[0] if self.get_widget(1).value else 0

        # Store search information in parent app
        self.parentApp.search_query = query
        self.parentApp.search_type = search_type

        # Switch back to main form
        self.parentApp.switchForm("MAIN")

        # Let the main form know to process the search
        self.parentApp.needs_search = True

    def afterEditing(self):
        self.parentApp.switchForm("MAIN")

# Main application form


class OSINTMainForm(npyscreen.FormBaseNewWithMenus):
    def create(self):
        # Setup colors
        setup_colors()

        # Add the menu
        self.menu = self.new_menu(name="Main Menu", shortcut="^M")
        self.menu.addItem("Load Database", self.on_load_database, "^L")
        self.menu.addItem("Build Database", self.on_build_database, "^B")
        self.menu.addItem("Search", self.on_search, "^S")
        self.menu.addItem("View Usage Stats", self.on_view_stats, "^U")
        self.menu.addItem("Generate Workflow", self.on_generate_workflow, "^W")
        self.menu.addItem("Exit", self.on_exit, "^X")

        # Get terminal size
        max_y, max_x = self.curses_pad.getmaxyx()

        # Add title
        self.add(npyscreen.TitleText, name="VANTAGE OSINT Intelligence", value="",
                 editable=False, color="IMPORTANT", rely=1)

        # Add status box at bottom
        status_box = self.add(ColorBoxTitle, name="Status", relx=2, rely=max_y - 6,
                              max_height=5, max_width=max_x - 4, color="GOOD")
        self.status_display = self.add(npyscreen.MultiLine,
                                       relx=3, rely=max_y - 5,
                                       max_height=3, max_width=max_x - 6,
                                       values=["Welcome to VANTAGE OSINT Intelligence",
                                               "Press ^M for menu or use buttons below"],
                                       editable=False, parent=status_box)

        # Create boxes for different sections
        tool_box_height = max_y - 15
        tool_box = self.add(ColorBoxTitle, name="OSINT Tools", relx=2, rely=4,
                            max_height=tool_box_height, max_width=max_x // 2 - 3, color="LABEL")

        # Tool list
        self.tool_list = self.add(ColoredMultiLine, relx=3, rely=5,
                                  max_height=tool_box_height - 2, max_width=max_x // 2 - 5,
                                  values=[], parent=tool_box)
        self.tool_list.add_handlers({
            curses.ascii.CR: self.on_tool_selected,
            curses.ascii.NL: self.on_tool_selected,
            "^S": self.on_search
        })

        # Create suggestion box
        suggestion_box = self.add(ColorBoxTitle, name="Suggestions", relx=max_x // 2, rely=4,
                                  max_height=tool_box_height, max_width=max_x // 2 - 2, color="LABEL")

        # Suggestion list
        self.suggestion_list = self.add(ColoredMultiLine, relx=max_x // 2 + 1, rely=5,
                                        max_height=tool_box_height - 2, max_width=max_x // 2 - 4,
                                        values=[], parent=suggestion_box)
        self.suggestion_list.add_handlers({
            curses.ascii.CR: self.on_suggestion_selected,
            curses.ascii.NL: self.on_suggestion_selected
        })

        # Add buttons at the bottom
        button_y = max_y - 10
        self.add(ColorButton, name="Search", relx=10, rely=button_y,
                 when_pressed_function=self.on_search, bg_color=1)
        self.add(ColorButton, name="View Tool", relx=25, rely=button_y,
                 when_pressed_function=self.on_view_tool, bg_color=1)
        self.add(ColorButton, name="Workflow", relx=40, rely=button_y,
                 when_pressed_function=self.on_generate_workflow, bg_color=1)
        self.add(ColorButton, name="Refresh", relx=55, rely=button_y,
                 when_pressed_function=self.on_refresh, bg_color=1)
        self.add(ColorButton, name="Exit", relx=70, rely=button_y,
                 when_pressed_function=self.on_exit, bg_color=6)

        # Initialize data
        self.tools_db = {}
        self.osint_tools = []
        self.suggestions = []

        # Set initial status
        self.status("Loading OSINT tool database...")
        self.on_load_database()

    def status(self, message):
        """Update the status display with a new message"""
        current_time = datetime.now().strftime("%H:%M:%S")
        values = self.status_display.values
        if len(values) >= 3:
            values.pop(0)
        values.append(f"[{current_time}] {message}")
        self.status_display.values = values
        self.status_display.display()

    def on_load_database(self):
        """Load the OSINT tool database"""
        def load_thread():
            try:
                self.tools_db = load_tool_database()

                # Convert to OSINTTool objects
                self.osint_tools = []
                for name, info in self.tools_db.items():
                    self.osint_tools.append(OSINTTool(name, info))

                # Sort by category and then name
                self.osint_tools.sort(key=lambda x: (x.category, x.name))

                # Update the UI
                self.update_lists()
                self.status(f"Loaded {len(self.osint_tools)} OSINT tools from database")
            except Exception as e:
                self.status(f"Error loading database: {str(e)}")

        # Run in a separate thread to avoid blocking the UI
        threading.Thread(target=load_thread).start()

    def on_build_database(self):
        """Build or update the OSINT tool database"""
        def build_thread():
            try:
                self.status("Building OSINT tool database...")
                build_database()
                self.status("Database build complete, reloading...")
                time.sleep(1)
                self.on_load_database()
            except Exception as e:
                self.status(f"Error building database: {str(e)}")

        # Run in a separate thread to avoid blocking the UI
        threading.Thread(target=build_thread).start()

    def on_search(self):
        """Open the search form"""
        self.parentApp.switchForm("SEARCH")

    def on_view_stats(self):
        """View usage statistics"""
        def stats_thread():
            try:
                self.status("Loading usage statistics...")
                usage_data = parse_usage_log()
                stats = calculate_stats(usage_data)

                # Create a message with the statistics
                stat_lines = [
                    f"Total uses: {stats['total_uses']}",
                    f"Unique tools: {stats['unique_tools']}",
                    "Most used tools:"
                ]
                for tool, count in stats['most_used']:
                    stat_lines.append(f"  {tool}: {count} uses")

                stat_lines.append("Recently used tools:")
                for tool in stats['recent_tools'][:5]:
                    stat_lines.append(f"  {tool}")

                # Display the statistics
                npyscreen.notify_confirm(message="\n".join(stat_lines),
                                         title="OSINT Tool Usage Statistics",
                                         form_color='STANDOUT',
                                         wrap=True,
                                         wide=True)

                self.status("Displayed usage statistics")
            except Exception as e:
                self.status(f"Error loading statistics: {str(e)}")

        # Run in a separate thread to avoid blocking the UI
        threading.Thread(target=stats_thread).start()

    def on_generate_workflow(self):
        """Generate an OSINT workflow"""
        def get_input():
            # Ask for the investigation type
            investigation_type = npyscreen.notify_input(
                "Enter investigation type (e.g., 'person investigation', 'domain recon'):",
                title="Generate Workflow")
            if investigation_type:
                return investigation_type
            return None

        investigation_type = get_input()
        if not investigation_type:
            self.status("Workflow generation cancelled")
            return

        def workflow_thread(investigation_type):
            try:
                self.status(f"Generating workflow for: {investigation_type}...")
                workflow_data = generate_workflow(investigation_type)

                # Store workflow data in parent app
                self.parentApp.workflow_data = workflow_data

                # Switch to workflow info form
                self.parentApp.switchForm("WORKFLOW")

                self.status(f"Generated workflow for {investigation_type}")
            except Exception as e:
                self.status(f"Error generating workflow: {str(e)}")

        # Run in a separate thread to avoid blocking the UI
        threading.Thread(target=workflow_thread, args=(investigation_type,)).start()

    def on_exit(self):
        """Exit the application"""
        self.parentApp.switchForm(None)

    def on_refresh(self):
        """Refresh the tool list and suggestions"""
        self.status("Refreshing display...")
        self.update_lists()
        self.status("Display refreshed")

    def on_tool_selected(self, *args):
        """Handle tool selection"""
        if self.tool_list.value is not None and self.tool_list.value < len(self.osint_tools):
            selected_tool = self.osint_tools[self.tool_list.value]

            # Store the selected tool in the parent app
            self.parentApp.selected_tool = selected_tool

            # Switch to the tool info form
            self.parentApp.switchForm("TOOLINFO")

    def on_view_tool(self):
        """View details of the selected tool"""
        self.on_tool_selected()

    def on_suggestion_selected(self, *args):
        """Handle suggestion selection"""
        if self.suggestion_list.value is not None and self.suggestion_list.value < len(self.suggestions):
            suggestion = self.suggestions[self.suggestion_list.value]

            # Find the matching tool
            for tool in self.osint_tools:
                if tool.name == suggestion.name:
                    # Store the selected tool in the parent app
                    self.parentApp.selected_tool = tool

                    # Switch to the tool info form
                    self.parentApp.switchForm("TOOLINFO")
                    break

    def update_lists(self):
        """Update the tool and suggestion lists"""
        # Update tool list
        self.tool_list.values = self.osint_tools

        # Update suggestion list based on available tools
        self.update_suggestions()

        # Ensure the display is updated
        self.tool_list.display()
        self.suggestion_list.display()

    def update_suggestions(self):
        """Update the suggestions list"""
        # Check if we need to process a search
        if hasattr(self.parentApp, 'needs_search') and self.parentApp.needs_search:
            self.process_search()
            self.parentApp.needs_search = False
            return

        if not self.tools_db:
            self.suggestions = []
            self.suggestion_list.values = ["No tools loaded yet"]
            return

        try:
            # Get suggestions based on usage
            usage_data = parse_usage_log()
            suggestion_data = suggest_tools(usage_data=usage_data, tools_db=self.tools_db)

            self.suggestions = []
            suggestion_messages = []

            if 'based_on' in suggestion_data:
                # We have suggestions based on recent usage
                based_on = suggestion_data['based_on']
                data_types = suggestion_data['handled_data_types']

                suggestion_messages.append(f"Based on your use of: {based_on}")
                suggestion_messages.append(f"That tool handles: {', '.join(data_types)}")
                suggestion_messages.append("")
                suggestion_messages.append("Suggested tools for similar tasks:")

                # Add suggestions
                for tool in suggestion_data.get('suggested_tools', []):
                    tool_obj = OSINTTool(tool['name'], {
                        'description': tool.get('description', ''),
                        'category': tool.get('category', 'unknown'),
                        'data_types': [],
                        'url': tool.get('url', '')
                    })
                    self.suggestions.append(tool_obj)
                    suggestion_messages.append(f"- {tool['name']} ({tool.get('category', 'unknown')})")

                # Add related tools heading
                if suggestion_data.get('commonly_used_with'):
                    suggestion_messages.append("")
                    suggestion_messages.append("Commonly used with:")

                    for tool, count in suggestion_data.get('commonly_used_with', []):
                        # Find the tool in the database
                        if tool in self.tools_db:
                            tool_obj = OSINTTool(tool, self.tools_db[tool])
                            if tool_obj not in self.suggestions:
                                self.suggestions.append(tool_obj)
                            suggestion_messages.append(f"- {tool} (used together {count} times)")

                # Add similar tools heading
                if suggestion_data.get('similar_tools'):
                    suggestion_messages.append("")
                    suggestion_messages.append("Similar tools by functionality:")

                    for tool in suggestion_data.get('similar_tools', []):
                        tool_obj = OSINTTool(tool['name'], {
                            'description': tool.get('description', ''),
                            'similarity': tool.get('similarity', 0)
                        })
                        if tool_obj not in self.suggestions:
                            self.suggestions.append(tool_obj)
                        suggestion_messages.append(f"- {tool['name']} (similarity: {tool['similarity']:.2f})")

            elif 'popular_tools' in suggestion_data:
                # Suggest popular tools
                suggestion_messages.append("Popular Tools from Your GitHub Stars:")
                suggestion_messages.append("")

                for tool in suggestion_data.get('popular_tools', []):
                    tool_obj = OSINTTool(tool['name'], {
                        'description': tool.get('description', ''),
                        'stars': tool.get('stars', 0)
                    })
                    self.suggestions.append(tool_obj)
                    suggestion_messages.append(f"- {tool['name']} ({tool.get('stars', 0)} stars)")

            self.suggestion_list.values = suggestion_messages
        except Exception as e:
            self.status(f"Error updating suggestions: {str(e)}")
            self.suggestion_list.values = [f"Error updating suggestions: {str(e)}"]

    def process_search(self):
        """Process a search query from the search form"""
        query = self.parentApp.search_query
        search_type = self.parentApp.search_type

        if not query:
            self.status("No search query provided")
            return

        self.status(f"Processing search: '{query}'")

        try:
            if search_type == 0:  # Data Type Search
                self.status(f"Searching for tools handling data type: {query}")
                matching_tools = find_tools_by_data_type(query, self.tools_db)

                self.suggestions = []
                suggestion_messages = [f"OSINT Tools for data type: {query}", ""]

                if not matching_tools:
                    suggestion_messages.append(f"No tools found that handle {query}.")
                else:
                    for tool in matching_tools:
                        tool_obj = OSINTTool(tool['name'], {
                            'description': tool.get('description', ''),
                            'category': tool.get('category', 'unknown'),
                            'url': tool.get('url', '')
                        })
                        self.suggestions.append(tool_obj)
                        suggestion_messages.append(f"- {tool['name']} ({tool.get('category', 'unknown')})")
                        suggestion_messages.append(f"  {tool.get('description', '')}")
                        suggestion_messages.append("")

                self.suggestion_list.values = suggestion_messages

            elif search_type == 1:  # Task Suggestion
                self.status(f"Finding tools for task: {query}")
                suggestion_data = suggest_tools(query, tools_db=self.tools_db)

                self.suggestions = []
                suggestion_messages = [f"Suggested Tools for: {query}", ""]

                if 'suggested_tools' in suggestion_data:
                    for tool in suggestion_data['suggested_tools']:
                        tool_obj = OSINTTool(tool['name'], {
                            'description': tool.get('description', ''),
                            'category': tool.get('category', 'unknown'),
                            'data_types': tool.get('data_types', []),
                            'score': tool.get('score', 0),
                            'explanation': tool.get('explanation', '')
                        })
                        self.suggestions.append(tool_obj)
                        suggestion_messages.append(f"- {tool['name']} ({tool.get('category', 'unknown')})")
                        suggestion_messages.append(f"  {tool.get('description', '')}")

                        if 'data_types' in tool and tool['data_types']:
                            suggestion_messages.append(f"  Data types: {', '.join(tool['data_types'])}")

                        if 'explanation' in tool:
                            suggestion_messages.append(f"  Why it's useful: {tool['explanation']}")
                        elif 'score' in tool:
                            suggestion_messages.append(f"  Relevance score: {tool['score']:.2f}")

                        suggestion_messages.append("")

                self.suggestion_list.values = suggestion_messages

            elif search_type == 2:  # Workflow Generation
                self.status(f"Generating workflow for: {query}")
                workflow_data = generate_workflow(query)

                # Store workflow data in parent app
                self.parentApp.workflow_data = workflow_data

                # Switch to workflow info form
                self.parentApp.switchForm("WORKFLOW")

            self.status(f"Search complete: '{query}'")

        except Exception as e:
            self.status(f"Error processing search: {str(e)}")
            self.suggestion_list.values = [f"Error processing search: {str(e)}"]

    def while_waiting(self):
        """Called when the application is waiting for input"""
        # Update dynamic content if needed

# Main application class


class OSINTTUIApp(npyscreen.NPSAppManaged):
    def onStart(self):
        # Add forms
        self.addForm("MAIN", OSINTMainForm, name="VANTAGE OSINT Intelligence")
        self.addForm("TOOLINFO", ToolInfoForm, name="Tool Information")
        self.addForm("SEARCH", SearchForm, name="Search OSINT Tools")
        self.addForm("WORKFLOW", WorkflowInfoForm, name="OSINT Workflow")

        # Initialize variables
        self.selected_tool = None
        self.workflow_data = None
        self.search_query = None
        self.search_type = None
        self.needs_search = False

# Main function


def main():
    app = OSINTTUIApp()
    try:
        app.run()
    except KeyboardInterrupt:
        pass


if __name__ == "__main__":
    main()
