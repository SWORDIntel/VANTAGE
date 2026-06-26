#!/usr/bin/env python3
# vantage_gitstar.py: GitHub starred repositories analyzer for VANTAGE
# Downloads READMEs from starred GitHub repos and analyzes them with ML

# Standard library imports
import os
import sys
import json
import time
import argparse
import re
import hmac
import hashlib
import base64
from pathlib import Path
import importlib.util
from collections import defaultdict, Counter
from datetime import datetime

# Third-party imports (with robust error handling)
try:
    import requests
    import numpy as np
    from tqdm import tqdm
    DEPENDENCIES_MET = True
except ImportError as e:
    print(f"Missing dependency: {e}")
    print("Install requirements with: pip install requests beautifulsoup4 tqdm numpy scipy scikit-learn")
    DEPENDENCIES_MET = False

try:
    from sklearn.feature_extraction.text import TfidfVectorizer
    from sklearn.cluster import KMeans
    from sklearn.metrics.pairwise import cosine_similarity
    ML_AVAILABLE = True
except ImportError:
    print("Machine learning libraries not available. Basic features only.")
    ML_AVAILABLE = False

try:
    from llama_cpp import Llama
    LLM_AVAILABLE = True
except ImportError:
    print("llama-cpp-python not available, advanced analysis will be limited")
    LLM_AVAILABLE = False

# Try to import the context module for integration
CONTEXT_AVAILABLE = False
context_module = None
VANTAGE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONTEXT_MODULE_PATH = os.path.join(VANTAGE_DIR, "contrib", "vantage_context.py")

if os.path.exists(CONTEXT_MODULE_PATH):
    try:
        spec = importlib.util.spec_from_file_location("vantage_context", CONTEXT_MODULE_PATH)
        context_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(context_module)
        CONTEXT_AVAILABLE = True
    except Exception as e:
        print(f"Error loading vantage_context module: {e}")
        CONTEXT_AVAILABLE = False

# Constants
HOME_DIR = os.path.expanduser("~")
# Use gitstar/ in the project root for all GitStar data
PROJECT_ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
GITSTAR_DIR = os.path.join(PROJECT_ROOT, "gitstar")
READMES_DIR = os.path.join(GITSTAR_DIR, "readmes")
CACHE_DIR = os.path.join(GITSTAR_DIR, "cache")
MODEL_DIR = os.path.join(GITSTAR_DIR, "models")
DATA_FILE = os.path.join(GITSTAR_DIR, "repo_data.json")
CATEGORIES_FILE = os.path.join(GITSTAR_DIR, "categories.json")
VECTORS_FILE = os.path.join(GITSTAR_DIR, "readme_vectors.npz")
# Always use gitstar/models/ for LLM models
DEFAULT_MODEL_PATH = os.path.join(MODEL_DIR, "mistral-7b-instruct-v0.2.Q4_K_M.gguf")
GITHUB_API_URL = "https://api.github.com"
USER_AGENT = "VANTAGE-GitStar/1.0"

# Ensure directories exist in the new location
Path(READMES_DIR).mkdir(parents=True, exist_ok=True)
Path(CACHE_DIR).mkdir(parents=True, exist_ok=True)
Path(MODEL_DIR).mkdir(parents=True, exist_ok=True)


class GitHubStarAnalyzer:
    """Analyzes READMEs from starred GitHub repositories using ML techniques"""

    def __init__(self):
        self.repos_data = self._load_repos_data()
        self.categories = self._load_categories()
        self.vectorizer = None
        self.vectors = None
        self.clusters = None
        self.llm = None

        # Initialize ML components if available
        if ML_AVAILABLE:
            self._initialize_ml()

        # Initialize LLM if available
        if LLM_AVAILABLE:
            self._initialize_llm()

    def _load_repos_data(self):
        """Load repository data from JSON file"""
        if os.path.exists(DATA_FILE):
            try:
                with open(DATA_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return {"repositories": [], "last_updated": time.time()}
        return {"repositories": [], "last_updated": time.time()}

    def _load_categories(self):
        """Load repository categories from JSON file"""
        if os.path.exists(CATEGORIES_FILE):
            try:
                with open(CATEGORIES_FILE, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                return {"categories": {}, "last_updated": time.time()}
        return {"categories": {}, "last_updated": time.time()}

    def _initialize_ml(self):
        """Initialize machine learning components"""
        self.vectorizer = TfidfVectorizer(
            max_features=5000,
            stop_words='english',
            ngram_range=(1, 2),
            min_df=2,
            max_df=0.85
        )

        # Load pre-computed vectors if available
        if os.path.exists(VECTORS_FILE):
            try:
                data = np.load(VECTORS_FILE)
                self.vectors = data['vectors']
                self.clusters = data.get('clusters')
            except Exception as e:
                print(f"Error loading vectors: {e}")

    def _initialize_llm(self):
        """Initialize the LLM for advanced analysis"""
        if os.path.exists(DEFAULT_MODEL_PATH):
            try:
                self.llm = Llama(
                    model_path=DEFAULT_MODEL_PATH,
                    n_ctx=4096,
                    n_threads=4,
                    verbose=False
                )
                print(f"LLM initialized with {DEFAULT_MODEL_PATH}")
            except Exception as e:
                print(f"Error initializing LLM: {e}")
                self.llm = None
        else:
            print(f"Model file not found: {DEFAULT_MODEL_PATH}")

    def save_repos_data(self):
        """Save repository data to JSON file"""
        with open(DATA_FILE, "w") as f:
            json.dump(self.repos_data, f, indent=2)

    def save_categories(self):
        """Save repository categories to JSON file"""
        with open(CATEGORIES_FILE, "w") as f:
            json.dump(self.categories, f, indent=2)

    def _generate_token_for_auth(self, password):
        """Generate a secure token for GitHub API authentication"""
        salt = os.urandom(16)
        key = hashlib.pbkdf2_hmac('sha256', password.encode('utf-8'), salt, 100000)
        return base64.b64encode(salt + key).decode('utf-8')

    def _verify_token(self, token, password):
        """Verify a token against a password"""
        try:
            decoded = base64.b64decode(token.encode('utf-8'))
            salt, key = decoded[:16], decoded[16:]
            verification_key = hashlib.pbkdf2_hmac('sha256', password.encode('utf-8'), salt, 100000)
            return hmac.compare_digest(key, verification_key)
        except Exception:
            return False

    def fetch_starred_repos(self, username, password=None):
        """Fetch starred repositories for a given GitHub username"""
        if not DEPENDENCIES_MET:
            print("Dependencies not met. Cannot fetch repositories.")
            return False

        headers = {
            "User-Agent": USER_AGENT
        }

        # Authentication (if provided)
        auth = None
        if password:
            auth = (username, password)
            print("Using authenticated request")

        # Fetch starred repositories
        page = 1
        all_repos = []

        with tqdm(desc="Fetching starred repositories", unit="page") as pbar:
            while True:
                try:
                    response = requests.get(
                        f"{GITHUB_API_URL}/users/{username}/starred",
                        params={"page": page, "per_page": 100},
                        headers=headers,
                        auth=auth
                    )

                    if response.status_code == 200:
                        repos = response.json()
                        if not repos:
                            break

                        all_repos.extend(repos)
                        page += 1
                        pbar.update(1)
                        pbar.set_description(f"Fetched {len(all_repos)} repositories")
                    else:
                        print(f"Error: {response.status_code} - {response.text}")
                        break
                except Exception as e:
                    print(f"Error fetching repositories: {e}")
                    break

        # Process and store the repositories
        processed_repos = []
        for repo in all_repos:
            processed_repos.append({
                "id": repo["id"],
                "name": repo["name"],
                "full_name": repo["full_name"],
                "owner": repo["owner"]["login"],
                "html_url": repo["html_url"],
                "description": repo["description"],
                "language": repo["language"],
                "stars": repo["stargazers_count"],
                "forks": repo["forks_count"],
                "created_at": repo["created_at"],
                "updated_at": repo["updated_at"],
                "readme_path": None,
                "readme_downloaded": False,
                "analyzed": False
            })

        # Update repos data
        self.repos_data["repositories"] = processed_repos
        self.repos_data["last_updated"] = time.time()
        self.save_repos_data()

        print(f"Successfully fetched {len(processed_repos)} starred repositories for {username}")

        # Download READMEs
        self.download_readmes()

        return True

    def download_readmes(self):
        """Download README files for all repositories"""
        if not DEPENDENCIES_MET:
            print("Dependencies not met. Cannot download READMEs.")
            return False

        headers = {
            "User-Agent": USER_AGENT
        }

        repos_to_update = [repo for repo in self.repos_data["repositories"]
                           if not repo.get("readme_downloaded", False)]

        with tqdm(repos_to_update, desc="Downloading READMEs", unit="repo") as pbar:
            for repo in pbar:
                try:
                    # Try to get the README.md file
                    response = requests.get(
                        f"https://raw.githubusercontent.com/{repo['full_name']}/master/README.md",
                        headers=headers
                    )

                    if response.status_code == 200:
                        # Save the README
                        readme_path = os.path.join(READMES_DIR, f"{repo['full_name'].replace('/', '_')}.md")
                        with open(readme_path, "w", encoding="utf-8") as f:
                            f.write(response.text)

                        # Update repository info
                        repo["readme_path"] = readme_path
                        repo["readme_downloaded"] = True
                        pbar.set_description(f"Downloaded README for {repo['full_name']}")
                    else:
                        # Try README.markdown
                        response = requests.get(
                            f"https://raw.githubusercontent.com/{repo['full_name']}/master/README.markdown",
                            headers=headers
                        )

                        if response.status_code == 200:
                            readme_path = os.path.join(READMES_DIR, f"{repo['full_name'].replace('/', '_')}.md")
                            with open(readme_path, "w", encoding="utf-8") as f:
                                f.write(response.text)

                            repo["readme_path"] = readme_path
                            repo["readme_downloaded"] = True
                            pbar.set_description(f"Downloaded README for {repo['full_name']}")
                        else:
                            # Try README
                            response = requests.get(
                                f"https://raw.githubusercontent.com/{repo['full_name']}/master/README",
                                headers=headers
                            )

                            if response.status_code == 200:
                                readme_path = os.path.join(READMES_DIR, f"{repo['full_name'].replace('/', '_')}.md")
                                with open(readme_path, "w", encoding="utf-8") as f:
                                    f.write(response.text)

                                repo["readme_path"] = readme_path
                                repo["readme_downloaded"] = True
                                pbar.set_description(f"Downloaded README for {repo['full_name']}")
                            else:
                                pbar.set_description(f"No README found for {repo['full_name']}")
                except Exception as e:
                    print(f"Error downloading README for {repo['full_name']}: {e}")

        # Save the updated repository data
        self.save_repos_data()

        # Count successful downloads
        downloaded_count = sum(1 for repo in self.repos_data["repositories"]
                               if repo.get("readme_downloaded", False))
        print(f"Successfully downloaded {downloaded_count} README files")

        return True

    def analyze_readmes(self):
        """Analyze README files using ML techniques"""
        if not ML_AVAILABLE:
            print("Machine learning libraries not available. Cannot perform analysis.")
            return False

        # Get repositories with downloaded READMEs
        repos_with_readme = [repo for repo in self.repos_data["repositories"]
                             if repo.get("readme_downloaded", False)]

        if not repos_with_readme:
            print("No README files available for analysis.")
            return False

        # Read README contents
        readme_contents = []
        readme_ids = []

        for repo in repos_with_readme:
            try:
                with open(repo["readme_path"], "r", encoding="utf-8") as f:
                    content = f.read()
                    readme_contents.append(content)
                    readme_ids.append(repo["id"])
            except Exception as e:
                print(f"Error reading README for {repo['full_name']}: {e}")

        print(f"Analyzing {len(readme_contents)} README files...")

        # Vectorize README contents
        self.vectors = self.vectorizer.fit_transform(readme_contents)

        # Determine optimal number of clusters (max 20)
        n_repos = len(readme_contents)
        n_clusters = min(20, max(3, n_repos // 10))

        # Apply K-means clustering
        kmeans = KMeans(n_clusters=n_clusters, random_state=42, n_init=10)
        self.clusters = kmeans.fit_predict(self.vectors)

        # Save vectors and clusters for future use
        np.savez(VECTORS_FILE, vectors=self.vectors, clusters=self.clusters)

        # Extract and store most common terms for each cluster
        cluster_terms = {}
        for i in range(n_clusters):
            # Get indices of repositories in this cluster
            cluster_indices = [j for j, c in enumerate(self.clusters) if c == i]

            # Get the repository information for this cluster
            cluster_repos = [readme_ids[j] for j in cluster_indices]

            # Get the top features for this cluster
            if cluster_indices:
                cluster_vectors = self.vectors[cluster_indices]
                centroid = cluster_vectors.mean(axis=0)

                # Get feature names
                feature_names = self.vectorizer.get_feature_names_out()

                # Robust conversion for both sparse and dense matrices
                centroid_arr = np.asarray(centroid).flatten()
                top_indices = centroid_arr.argsort()[-20:][::-1]
                top_terms = [feature_names[idx] for idx in top_indices]

                cluster_terms[str(i)] = {
                    "terms": top_terms,
                    "repos": cluster_repos
                }

        # Update categories
        self.categories["categories"] = cluster_terms
        self.categories["last_updated"] = time.time()
        self.save_categories()

        # Update repositories with cluster information
        for i, repo_id in enumerate(readme_ids):
            for repo in self.repos_data["repositories"]:
                if repo["id"] == repo_id:
                    repo["cluster"] = int(self.clusters[i])
                    repo["analyzed"] = True

        self.save_repos_data()

        # Use LLM for advanced analysis if available
        if LLM_AVAILABLE and self.llm:
            self.analyze_with_llm()

        print("Analysis complete. Repositories categorized into clusters.")
        return True

    def analyze_with_llm(self):
        """Use LLM to analyze repositories for more detailed insights"""
        if not LLM_AVAILABLE or not self.llm:
            print("LLM not available for advanced analysis.")
            return False

        print("Performing advanced analysis with LLM...")

        # Get repositories with downloaded READMEs
        repos_with_readme = [repo for repo in self.repos_data["repositories"]
                             if repo.get("readme_downloaded", False)]

        # Sample repositories from each cluster for analysis
        cluster_to_repos = defaultdict(list)
        for repo in repos_with_readme:
            if repo.get("analyzed", False) and "cluster" in repo:
                cluster_to_repos[repo["cluster"]].append(repo)

        # Process a representative subset from each cluster
        analyzed_repos = 0
        len(cluster_to_repos)

        for cluster_id, repos in tqdm(cluster_to_repos.items(), desc="Analyzing clusters"):
            # Select up to 5 repos from each cluster for detailed analysis
            sample_repos = repos[:5]

            for repo in tqdm(sample_repos, desc=f"Cluster {cluster_id}", leave=False):
                if repo.get("llm_analyzed", False):
                    continue

                try:
                    # Read README content
                    with open(repo["readme_path"], "r", encoding="utf-8") as f:
                        readme_content = f.read()

                    # Truncate if too long
                    max_chars = 8000  # Limit due to context window
                    if len(readme_content) > max_chars:
                        readme_content = readme_content[:max_chars] + "..."

                    # Prepare prompt for LLM
                    prompt = f"""
                    Analyze the following GitHub repository README:

                    Repository: {repo['full_name']}
                    Language: {repo['language'] or 'Unknown'}

                    README:
                    ```
                    {readme_content}
                    ```

                    Task:
                    1. Identify the main purpose of this repository in one sentence.
                    2. List the key features of this software.
                    3. Categorize this repository (e.g., "Web Framework", "Data Visualization", "DevOps Tool", etc.)
                    4. Rate the documentation quality from 1-5.
                    5. List potential use cases for this software.

                    Format your response in valid JSON as follows:
                    {{
                        "purpose": "Brief one-sentence description",
                        "features": ["feature1", "feature2", ...],
                        "category": "Primary category",
                        "subcategories": ["subcategory1", "subcategory2"],
                        "doc_quality": rating_number,
                        "use_cases": ["case1", "case2", ...],
                        "complexity": "Beginner|Intermediate|Advanced"
                    }}
                    """

                    # Generate analysis with LLM
                    output = self.llm(
                        prompt,
                        max_tokens=1024,
                        temperature=0.3,
                        stop=["```"],
                        echo=False
                    )

                    # Extract JSON from response
                    result_text = output["choices"][0]["text"].strip()

                    # Find JSON content
                    json_match = re.search(r'({[\s\S]*})', result_text)
                    if json_match:
                        try:
                            analysis = json.loads(json_match.group(1))

                            # Store analysis in repository data
                            repo["llm_analysis"] = analysis
                            repo["llm_analyzed"] = True
                            analyzed_repos += 1
                        except json.JSONDecodeError as e:
                            print(f"Error parsing LLM output: {e}")
                except Exception as e:
                    print(f"Error analyzing {repo['full_name']}: {e}")

        # Save updated repository data
        self.save_repos_data()

        # Update category names based on LLM analysis
        self.update_category_names()

        print(f"LLM analysis complete for {analyzed_repos} repositories.")
        return True

    def update_category_names(self):
        """Update cluster names based on LLM analysis"""
        if not self.categories["categories"]:
            return

        # Map repository IDs to their analysis
        id_to_analysis = {}
        for repo in self.repos_data["repositories"]:
            if repo.get("llm_analyzed", False) and "llm_analysis" in repo:
                id_to_analysis[repo["id"]] = repo["llm_analysis"]

        # Update each cluster with a meaningful name
        for cluster_id, cluster_data in self.categories["categories"].items():
            # Get analyses for repositories in this cluster
            analyses = [id_to_analysis[repo_id] for repo_id in cluster_data["repos"]
                        if repo_id in id_to_analysis]

            if analyses:
                # Count categories
                categories = [analysis.get("category", "Unknown") for analysis in analyses]
                category_counter = Counter(categories)
                most_common = category_counter.most_common(1)[0][0]

                # Count subcategories
                subcategories = []
                for analysis in analyses:
                    subcategories.extend(analysis.get("subcategories", []))
                subcategory_counter = Counter(subcategories)
                top_subcategories = [sc for sc, _ in subcategory_counter.most_common(3)]

                # Update cluster information
                cluster_data["name"] = most_common
                cluster_data["subcategories"] = top_subcategories

        # Save updated categories
        self.save_categories()

    def search_repositories(self, query):
        """Search for repositories matching the query"""
        if not self.repos_data["repositories"]:
            print("No repositories available. Fetch repositories first.")
            return []

        # Ensure query is lowercase for case-insensitive search
        query = query.lower()

        # Search in repository names and descriptions
        basic_matches = []
        for repo in self.repos_data["repositories"]:
            name = repo["name"].lower()
            full_name = repo["full_name"].lower()
            description = (repo["description"] or "").lower()

            score = 0
            if query in name:
                score += 3
            if query in full_name:
                score += 2
            if query in description:
                score += 1

            if score > 0:
                basic_matches.append((repo, score))

        # If ML is available, use vectorization for semantic search
        if ML_AVAILABLE and self.vectors is not None and self.vectorizer is not None:
            # Get repositories with downloaded READMEs
            repos_with_readme = [repo for repo in self.repos_data["repositories"]
                                 if repo.get("readme_downloaded", False)]

            readme_ids = [repo["id"] for repo in repos_with_readme]

            # Vectorize the query
            query_vector = self.vectorizer.transform([query])

            # Calculate similarity with all README vectors
            similarities = cosine_similarity(query_vector, self.vectors).flatten()

            # Get matching repositories
            semantic_matches = []
            for i, sim in enumerate(similarities):
                if sim > 0.1:  # Threshold for semantic similarity
                    repo_id = readme_ids[i]
                    repo = next((r for r in repos_with_readme if r["id"] == repo_id), None)
                    if repo:
                        semantic_matches.append((repo, sim * 5))  # Scale the score

            # Combine and deduplicate results
            all_matches = basic_matches + semantic_matches
            seen_ids = set()
            unique_matches = []

            for repo, score in all_matches:
                if repo["id"] not in seen_ids:
                    seen_ids.add(repo["id"])
                    unique_matches.append((repo, score))
                else:
                    # Update score if this is a duplicate
                    for i, (r, s) in enumerate(unique_matches):
                        if r["id"] == repo["id"]:
                            unique_matches[i] = (r, max(s, score))
                            break

            # Sort by score in descending order
            sorted_matches = sorted(unique_matches, key=lambda x: x[1], reverse=True)
            return [repo for repo, _ in sorted_matches]

        # If ML not available, just use basic matches
        sorted_matches = sorted(basic_matches, key=lambda x: x[1], reverse=True)
        return [repo for repo, _ in sorted_matches]

    def suggest_repositories(self, task_description):
        """Suggest repositories for a specific task"""
        if not self.repos_data["repositories"]:
            print("No repositories available. Fetch repositories first.")
            return []

        suggestions = []

        # If LLM is available, use it for advanced suggestions
        if LLM_AVAILABLE and self.llm:
            # Get repositories with LLM analysis
            analyzed_repos = [repo for repo in self.repos_data["repositories"]
                              if repo.get("llm_analyzed", False)]

            # Prepare prompt for LLM
            repo_descriptions = "\n".join([
                f"{i+1}. {repo['full_name']}: {repo.get('llm_analysis', {}).get('purpose', 'Unknown purpose')}"
                for i, repo in enumerate(analyzed_repos[:50])  # Limit to 50 to fit context
            ])

            prompt = f"""
            I need to {task_description}

            Here are some GitHub repositories I have starred:
            {repo_descriptions}

            Which numbered repositories would be most useful for this task? List the top 5 matches with their numbers and a brief explanation of why each is suitable.
            Format your response as a valid JSON array of objects with 'id', 'name', and 'reason' fields.
            """

            # Generate suggestions with LLM
            try:
                output = self.llm(
                    prompt,
                    max_tokens=1024,
                    temperature=0.3,
                    stop=["```"],
                    echo=False
                )

                # Extract JSON from response
                result_text = output["choices"][0]["text"].strip()

                # Find JSON content
                json_match = re.search(r'(\[[\s\S]*\])', result_text)
                if json_match:
                    try:
                        llm_suggestions = json.loads(json_match.group(1))

                        # Convert to actual repository objects
                        for suggestion in llm_suggestions:
                            if "id" in suggestion and 1 <= suggestion["id"] <= len(analyzed_repos):
                                repo = analyzed_repos[suggestion["id"] - 1]
                                suggestions.append({
                                    "repository": repo,
                                    "reason": suggestion.get("reason", "Matches your task requirements"),
                                    "score": 1.0 - (suggestion["id"] * 0.1)  # Higher score for lower IDs
                                })
                    except json.JSONDecodeError:
                        # Fallback to basic search if JSON parsing fails
                        print("Error parsing LLM suggestions. Using basic search instead.")
                        suggestions = self._basic_suggest(task_description)
                else:
                    suggestions = self._basic_suggest(task_description)
            except Exception as e:
                print(f"Error generating LLM suggestions: {e}")
                suggestions = self._basic_suggest(task_description)
        else:
            # Fallback to basic search
            suggestions = self._basic_suggest(task_description)

        # Sort by score in descending order
        sorted_suggestions = sorted(suggestions, key=lambda x: x["score"], reverse=True)
        return sorted_suggestions

    def _basic_suggest(self, task_description):
        """Basic suggestion without LLM"""
        # Use search as a fallback
        matches = self.search_repositories(task_description)

        suggestions = []
        for i, repo in enumerate(matches[:10]):  # Top 10 matches
            suggestions.append({
                "repository": repo,
                "reason": f"Matches keywords in '{task_description}'",
                "score": 1.0 - (i * 0.1)  # Higher score for earlier matches
            })

        return suggestions

    def get_statistics(self):
        """Get statistics about downloaded repositories"""
        if not self.repos_data["repositories"]:
            return {
                "total_repos": 0,
                "readmes_downloaded": 0,
                "analyzed_repos": 0,
                "llm_analyzed": 0,
                "languages": {},
                "clusters": {},
                "last_updated": None
            }

        # Count repositories
        total_repos = len(self.repos_data["repositories"])
        readmes_downloaded = sum(1 for repo in self.repos_data["repositories"]
                                 if repo.get("readme_downloaded", False))
        analyzed_repos = sum(1 for repo in self.repos_data["repositories"]
                             if repo.get("analyzed", False))
        llm_analyzed = sum(1 for repo in self.repos_data["repositories"]
                           if repo.get("llm_analyzed", False))

        # Count languages
        languages = defaultdict(int)
        for repo in self.repos_data["repositories"]:
            lang = repo.get("language") or "Unknown"
            languages[lang] += 1

        # Count clusters
        clusters = defaultdict(int)
        for repo in self.repos_data["repositories"]:
            if repo.get("analyzed", False) and "cluster" in repo:
                clusters[str(repo["cluster"])] += 1

        # Get cluster names
        cluster_names = {}
        if self.categories and "categories" in self.categories:
            for cluster_id, cluster_data in self.categories["categories"].items():
                name = cluster_data.get("name") or f"Cluster {cluster_id}"
                cluster_names[cluster_id] = {"name": name, "count": clusters.get(cluster_id, 0)}

        # Format last updated date
        last_updated = None
        if "last_updated" in self.repos_data:
            last_updated = datetime.fromtimestamp(self.repos_data["last_updated"]).strftime("%Y-%m-%d %H:%M:%S")

        return {
            "total_repos": total_repos,
            "readmes_downloaded": readmes_downloaded,
            "analyzed_repos": analyzed_repos,
            "llm_analyzed": llm_analyzed,
            "languages": dict(sorted(languages.items(), key=lambda x: x[1], reverse=True)),
            "clusters": cluster_names,
            "last_updated": last_updated
        }

# Main CLI interface


def main():
    """Main function for CLI interface"""
    parser = argparse.ArgumentParser(description="VANTAGE GitHub Star Analyzer")

    # Add arguments
    parser.add_argument("--fetch", metavar="USERNAME", help="Fetch starred repositories for a GitHub username")
    parser.add_argument("--password", help="GitHub password for authentication (optional)")
    parser.add_argument("--analyze", action="store_true", help="Analyze downloaded README files")
    parser.add_argument("--search", metavar="QUERY", help="Search for repositories matching the query")
    parser.add_argument("--suggest", metavar="TASK", help="Suggest repositories for a specific task")
    parser.add_argument("--stats", action="store_true", help="Show statistics about downloaded repositories")
    parser.add_argument("--update", metavar="USERNAME", help="Update repositories for a GitHub username")
    parser.add_argument("--verbose", action="store_true", help="Show verbose output")

    args = parser.parse_args()

    # Check if dependencies are met
    if not DEPENDENCIES_MET:
        print("Required dependencies missing. Please install them first.")
        print("pip install requests beautifulsoup4 tqdm numpy scipy scikit-learn")
        return 1

    # Initialize the analyzer
    analyzer = GitHubStarAnalyzer()

    # Handle commands
    if args.fetch:
        analyzer.fetch_starred_repos(args.fetch, args.password)

    elif args.analyze:
        analyzer.analyze_readmes()

    elif args.search:
        results = analyzer.search_repositories(args.search)
        if results:
            print(f"Found {len(results)} matching repositories:")
            for i, repo in enumerate(results[:20]):  # Limit to top 20
                print(f"{i+1}. {repo['full_name']} - {repo['description'] or 'No description'}")
                if args.verbose and "llm_analysis" in repo:
                    print(f"   Purpose: {repo['llm_analysis'].get('purpose', 'Unknown')}")
                    print(f"   Category: {repo['llm_analysis'].get('category', 'Unknown')}")
                    print("")
        else:
            print("No matching repositories found.")

    elif args.suggest:
        suggestions = analyzer.suggest_repositories(args.suggest)
        if suggestions:
            print(f"Top {len(suggestions)} suggested repositories for '{args.suggest}':")
            for i, suggestion in enumerate(suggestions):
                repo = suggestion["repository"]
                print(f"{i+1}. {repo['full_name']} - {repo['description'] or 'No description'}")
                print(f"   Reason: {suggestion['reason']}")
                if args.verbose and "llm_analysis" in repo:
                    print(f"   Purpose: {repo['llm_analysis'].get('purpose', 'Unknown')}")
                    print(f"   Features: {', '.join(repo['llm_analysis'].get('features', ['Unknown'])[:3])}")
                print("")
        else:
            print("No suitable repositories found.")

    elif args.stats:
        stats = analyzer.get_statistics()
        print("\nGitHub Starred Repository Statistics:")
        print(f"Total repositories: {stats['total_repos']}")
        print(f"READMEs downloaded: {stats['readmes_downloaded']}")
        print(f"Analyzed repositories: {stats['analyzed_repos']}")
        print(f"LLM-analyzed repositories: {stats['llm_analyzed']}")

        print("\nTop Languages:")
        for lang, count in list(stats['languages'].items())[:10]:  # Top 10 languages
            print(f"  {lang}: {count}")

        if stats['clusters']:
            print("\nRepository Clusters:")
            for cluster_id, data in stats['clusters'].items():
                print(f"  {data['name']}: {data['count']} repositories")

        if stats['last_updated']:
            print(f"\nLast updated: {stats['last_updated']}")

    elif args.update:
        analyzer.fetch_starred_repos(args.update, args.password)
        analyzer.analyze_readmes()

    else:
        parser.print_help()

    return 0

# Module functions for bash integration


def fetch_starred_repos(username, password=None):
    """Fetch starred repositories for a GitHub username"""
    analyzer = GitHubStarAnalyzer()
    return analyzer.fetch_starred_repos(username, password)


def analyze_readmes():
    """Analyze downloaded README files"""
    analyzer = GitHubStarAnalyzer()
    return analyzer.analyze_readmes()


def search_repositories(query):
    """Search for repositories matching the query"""
    analyzer = GitHubStarAnalyzer()
    results = analyzer.search_repositories(query)

    if results:
        print(f"Found {len(results)} matching repositories:")
        for i, repo in enumerate(results[:20]):  # Limit to top 20
            print(f"{i+1}. {repo['full_name']} - {repo['description'] or 'No description'}")
    else:
        print("No matching repositories found.")

    return results


def suggest_repositories(task):
    """Suggest repositories for a specific task"""
    analyzer = GitHubStarAnalyzer()
    suggestions = analyzer.suggest_repositories(task)

    if suggestions:
        print(f"Top {len(suggestions)} suggested repositories for '{task}':")
        for i, suggestion in enumerate(suggestions):
            repo = suggestion["repository"]
            print(f"{i+1}. {repo['full_name']} - {repo['description'] or 'No description'}")
            print(f"   Reason: {suggestion['reason']}")
    else:
        print("No suitable repositories found.")

    return suggestions


def get_statistics():
    """Get statistics about downloaded repositories"""
    analyzer = GitHubStarAnalyzer()
    return analyzer.get_statistics()


if __name__ == "__main__":
    sys.exit(main())
