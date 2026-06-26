#!/usr/bin/env python3
# VANTAGE OSINT Tool - Intelligence Tool Suggestion System
# This script analyzes OSINT tool usage patterns and provides intelligent suggestions
# based on the user's own GitHub starred repositories

# Standard library imports
import os
import sys
import json
import argparse
import datetime
import re
from collections import Counter, defaultdict

# Third-party imports (with robust error handling)
try:
    from tqdm import tqdm
    from sklearn.feature_extraction.text import TfidfVectorizer
    from sklearn.metrics.pairwise import cosine_similarity
except ImportError as e:
    print(f"Missing dependency: {e}")
    print("Install with: pip install numpy tqdm requests scikit-learn scipy")
    sys.exit(1)

# LLM support (optional)
LLM_AVAILABLE = False
try:
    from llama_cpp import Llama
    LLM_AVAILABLE = True
except ImportError:
    pass

# Define paths
HOME = os.path.expanduser("~")
VANTAGE_DIR = os.path.join(HOME, ".vantage")
OSINT_DIR = os.path.join(VANTAGE_DIR, "osint")
HISTORY_DIR = os.path.join(OSINT_DIR, "history")
REPOS_DIR = os.path.join(OSINT_DIR, "repos")
MODELS_DIR = os.path.join(OSINT_DIR, "models")
CACHE_DIR = os.path.join(OSINT_DIR, "cache")
TOOL_DB_PATH = os.path.join(OSINT_DIR, "tools.json")
USAGE_LOG_PATH = os.path.join(HISTORY_DIR, "usage.log")
WORKFLOW_PATH = os.path.join(OSINT_DIR, "workflows.json")
MODEL_PATH = os.path.join(MODELS_DIR, "osint_model.npz")
VECTORIZER_PATH = os.path.join(MODELS_DIR, "vectorizer.pkl")
CONFIG_PATH = os.path.join(OSINT_DIR, "config.json")

# GitStar directories (to leverage existing starred repos)
GITSTAR_DIR = os.path.join(VANTAGE_DIR, "gitstar")
GITSTAR_READMES_DIR = os.path.join(GITSTAR_DIR, "readmes")
GITSTAR_DB_PATH = os.path.join(GITSTAR_DIR, "repositories.json")

# Common data types for OSINT tools (used for categorization)
DATA_TYPE_KEYWORDS = {
    "email": ["email", "mail", "@", "smtp", "imap", "pop3"],
    "domain": ["domain", "dns", "url", "website", "site", "subdomain", "whois"],
    "ip": ["ip", "ipv4", "ipv6", "address", "cidr", "network", "subnet"],
    "username": ["username", "user", "account", "profile", "handle"],
    "phone": ["phone", "mobile", "cell", "number", "sms", "call"],
    "social": ["social", "facebook", "twitter", "instagram", "linkedin", "reddit"],
    "person": ["person", "people", "individual", "name", "identity"],
    "document": ["document", "pdf", "docx", "file", "spreadsheet"],
    "image": ["image", "photo", "picture", "exif", "metadata", "camera"],
    "password": ["password", "hash", "crack", "brute", "force", "rainbow"],
    "location": ["location", "geo", "gps", "coordinate", "place", "map"],
    "blockchain": ["blockchain", "bitcoin", "crypto", "wallet", "transaction"]
}

# Predefined OSINT workflows (without specific tools - will be filled in from your repos)
OSINT_WORKFLOWS = {
    "person_investigation": [
        {"step": 1, "purpose": "Find social media accounts"},
        {"step": 2, "purpose": "Collect detailed profile information"},
        {"step": 3, "purpose": "Discover email addresses"},
        {"step": 4, "purpose": "Check which services use the email"},
        {"step": 5, "purpose": "Analyze associated phone numbers"}
    ],
    "domain_investigation": [
        {"step": 1, "purpose": "Map attack surface and discover assets"},
        {"step": 2, "purpose": "Find subdomains"},
        {"step": 3, "purpose": "Gather emails and names"},
        {"step": 4, "purpose": "Identify technologies used"},
        {"step": 5, "purpose": "Extract metadata from documents"}
    ],
    "leak_investigation": [
        {"step": 1, "purpose": "Check account existence on sites"},
        {"step": 2, "purpose": "Crack password hashes if available"},
        {"step": 3, "purpose": "Find other accounts using same username"},
        {"step": 4, "purpose": "Search for mentions on social media"}
    ]
}

# Load GitStar repositories data


def load_gitstar_repositories():
    """Load the GitHub starred repositories data"""
    if os.path.exists(GITSTAR_DB_PATH):
        try:
            with open(GITSTAR_DB_PATH, 'r') as f:
                return json.load(f)
        except json.JSONDecodeError:
            print("Error loading GitStar repository database")
            return {}
    else:
        print("GitStar repository database not found. Please use vantage_gitstar_fetch first.")
        return {}

# Extract OSINT tools from GitHub starred repositories


def extract_tools_from_starred_repos():
    """Extract potential OSINT tools from GitHub starred repositories"""
    repos = load_gitstar_repositories()
    if not repos:
        print("No GitHub starred repositories found. Please run 'vantage_gitstar_fetch <username>' first.")
        return {}

    # OSINT-related keywords to identify relevant repositories
    osint_keywords = [
        "osint", "reconnaissance", "recon", "intelligence", "information gathering",
        "social media", "email", "username", "search", "scrape", "harvest",
        "dox", "scan", "analyze", "investigate", "discover", "track", "monitor",
        "locate", "metadata", "data mining", "crawl", "spider", "footprint",
        "security", "pentest", "penetration", "hack", "cyber", "forensic"
    ]

    # Extract OSINT tools from repositories
    tool_db = {}

    print("Analyzing GitHub starred repositories for OSINT tools...")
    for repo_name, repo_info in tqdm(repos.items(), desc="Processing repos"):
        # Skip repos without descriptions
        if not repo_info.get('description'):
            continue

        # Check if this repo is related to OSINT
        description = repo_info.get('description', '').lower()
        readme_content = ""

        # Try to read README content
        readme_path = os.path.join(GITSTAR_READMES_DIR, f"{repo_name}.md")
        if os.path.exists(readme_path):
            with open(readme_path, 'r', encoding='utf-8', errors='ignore') as f:
                readme_content = f.read().lower()

        # Check if this is an OSINT tool
        is_osint_tool = False
        matched_keywords = []
        for keyword in osint_keywords:
            if keyword in description or keyword in readme_content:
                is_osint_tool = True
                matched_keywords.append(keyword)
                if len(matched_keywords) >= 3:  # If we match 3+ keywords, it's very likely OSINT
                    break

        # If not identified as OSINT but has security indicators, add it anyway
        if not is_osint_tool and any(x in description.lower() for x in ["security", "pentest", "hacking"]):
            is_osint_tool = True
            matched_keywords.append("security")

        if not is_osint_tool:
            continue

        # Determine data types
        data_types = []
        for data_type, keywords in DATA_TYPE_KEYWORDS.items():
            for keyword in keywords:
                if keyword in description or keyword in readme_content:
                    if data_type not in data_types:
                        data_types.append(data_type)

        # If no data types identified, use "unknown"
        if not data_types:
            data_types = ["unknown"]

        # Determine category based on data types and description
        category = "unknown"
        if "email" in data_types:
            category = "email_gathering"
        elif "domain" in data_types or "ip" in data_types:
            category = "reconnaissance"
        elif "username" in data_types or "social" in data_types:
            category = "social_media"
        elif "password" in data_types:
            category = "password_cracking"
        elif "image" in data_types or "document" in data_types:
            category = "metadata_analysis"
        elif "location" in data_types:
            category = "geo_analysis"
        elif any(x in " ".join(matched_keywords) for x in ["recon", "intelligence", "gather"]):
            category = "reconnaissance"

        # Create tool entry
        tool_name = repo_name.split("/")[-1]
        tool_db[tool_name] = {
            "description": repo_info.get('description', 'No description available'),
            "data_types": data_types,
            "category": category,
            "url": f"https://github.com/{repo_name}",
            "stars": repo_info.get('stars', 0),
            "source": "github_starred",
            "matched_keywords": matched_keywords
        }

    return tool_db

# Load and initialize the tool database


def load_tool_database():
    """Load the OSINT tool database from file or initialize it from starred repos"""
    if os.path.exists(TOOL_DB_PATH):
        try:
            with open(TOOL_DB_PATH, 'r') as f:
                return json.load(f)
        except json.JSONDecodeError:
            print("Error loading tool database, initializing from starred repos")

    # Initialize with tools extracted from starred repos
    tool_db = extract_tools_from_starred_repos()

    # Save the database
    with open(TOOL_DB_PATH, 'w') as f:
        json.dump(tool_db, f, indent=4)

    return tool_db

# Save the tool database


def save_tool_database(tool_db):
    """Save the OSINT tool database to file"""
    with open(TOOL_DB_PATH, 'w') as f:
        json.dump(tool_db, f, indent=4)

# Parse the usage log to get tool usage history


def parse_usage_log():
    """Parse the usage log to get tool usage history"""
    usage_data = []
    if os.path.exists(USAGE_LOG_PATH):
        with open(USAGE_LOG_PATH, 'r') as f:
            for line in f:
                parts = line.strip().split('|')
                if len(parts) >= 3:
                    timestamp, cmd, directory = parts[0], parts[1], parts[2]
                    try:
                        dt = datetime.datetime.strptime(timestamp, "%Y-%m-%d %H:%M:%S")
                        usage_data.append({
                            'timestamp': timestamp,
                            'datetime': dt,
                            'command': cmd,
                            'directory': directory
                        })
                    except ValueError:
                        continue
    return usage_data

# Calculate tool usage statistics


def calculate_stats(usage_data):
    """Calculate statistics about tool usage"""
    if not usage_data:
        return {
            'total_uses': 0,
            'unique_tools': 0,
            'most_used': [],
            'recent_tools': [],
            'usage_by_day': {},
            'usage_by_category': {}
        }

    # Count tool usage
    tool_counts = Counter(item['command'] for item in usage_data)

    # Get most recent tools
    sorted_by_time = sorted(usage_data, key=lambda x: x['datetime'], reverse=True)
    recent_tools = [item['command'] for item in sorted_by_time[:10]]

    # Count by day
    usage_by_day = defaultdict(int)
    for item in usage_data:
        day = item['datetime'].strftime("%Y-%m-%d")
        usage_by_day[day] += 1

    # Count by category
    tools_db = load_tool_database()
    usage_by_category = defaultdict(int)
    for item in usage_data:
        tool = item['command']
        if tool in tools_db and 'category' in tools_db[tool]:
            category = tools_db[tool]['category']
            usage_by_category[category] += 1

    return {
        'total_uses': len(usage_data),
        'unique_tools': len(tool_counts),
        'most_used': tool_counts.most_common(5),
        'recent_tools': recent_tools,
        'usage_by_day': dict(usage_by_day),
        'usage_by_category': dict(usage_by_category)
    }

# Find tools that handle a specific data type


def find_tools_by_data_type(data_type, tools_db):
    """Find tools that can handle a specific data type"""
    matching_tools = []

    for tool_name, tool_info in tools_db.items():
        if 'data_types' in tool_info and data_type.lower() in [dt.lower() for dt in tool_info['data_types']]:
            matching_tools.append({
                'name': tool_name,
                'description': tool_info.get('description', ''),
                'category': tool_info.get('category', 'unknown'),
                'url': tool_info.get('url', '')
            })

    return matching_tools

# Get tools that are commonly used together


def get_related_tools(tool_name, usage_data, tools_db):
    """Find tools that are commonly used with the specified tool"""
    if not usage_data or tool_name not in tools_db:
        return []

    # Find sessions where this tool was used
    sessions = defaultdict(list)
    current_session = datetime.timedelta(hours=1)  # Tools used within 1 hour are in the same session

    for i, item in enumerate(sorted(usage_data, key=lambda x: x['datetime'])):
        session_day = item['datetime'].strftime("%Y-%m-%d")
        if not sessions[session_day]:
            sessions[session_day].append([item['command']])
        else:
            # Check if this command is within the current session timeframe
            last_session = sessions[session_day][-1]
            last_time = usage_data[i - 1]['datetime']
            if item['datetime'] - last_time <= current_session:
                last_session.append(item['command'])
            else:
                sessions[session_day].append([item['command']])

    # Find which tools are used in the same session as our target tool
    related_counts = Counter()
    for day, day_sessions in sessions.items():
        for session in day_sessions:
            if tool_name in session:
                for cmd in session:
                    if cmd != tool_name and cmd in tools_db:
                        related_counts[cmd] += 1

    return related_counts.most_common(5)

# Find tools with similar functionality based on descriptions


def find_similar_tools(tool_name, tools_db):
    """Find tools with similar functionality based on descriptions"""
    if tool_name not in tools_db:
        return []

    # Create TF-IDF vectorizer
    vectorizer = TfidfVectorizer(stop_words='english')

    # Create document corpus with tool descriptions
    documents = []
    tool_names = []
    for name, info in tools_db.items():
        doc = f"{name} {info.get('description', '')} "
        doc += ' '.join(info.get('data_types', []))
        doc += f" {info.get('category', '')}"
        documents.append(doc)
        tool_names.append(name)

    # Vectorize
    tfidf_matrix = vectorizer.fit_transform(documents)

    # Find the index of our target tool
    target_idx = tool_names.index(tool_name)

    # Calculate similarities between target and all tools
    target_vector = tfidf_matrix[target_idx:target_idx + 1]
    similarities = cosine_similarity(target_vector, tfidf_matrix)[0]

    # Get top 5 most similar tools (excluding the target itself)
    similar_tools = []
    sorted_indices = similarities.argsort()[::-1]

    # Skip the first index (it's the tool itself)
    for idx in sorted_indices[1:6]:
        similar_name = tool_names[idx]
        similar_tools.append({
            'name': similar_name,
            'description': tools_db[similar_name].get('description', ''),
            'similarity': float(similarities[idx])
        })

    return similar_tools

# Suggest tools based on recent usage


def suggest_tools(query=None, usage_data=None, tools_db=None):
    """Suggest OSINT tools based on usage history or a specific query"""
    if not tools_db:
        tools_db = load_tool_database()

    if not usage_data:
        usage_data = parse_usage_log()

    # If a query is provided, use it to suggest tools
    if query:
        return suggest_tools_for_task(query, tools_db)

    # If no query but we have usage data, suggest based on recency and patterns
    if usage_data:
        # Get most recent tool
        sorted_by_time = sorted(usage_data, key=lambda x: x['datetime'], reverse=True)
        if sorted_by_time:
            most_recent = sorted_by_time[0]['command']
            if most_recent in tools_db:
                # Get data types handled by this tool
                data_types = tools_db[most_recent].get('data_types', [])

                suggestions = []
                # Suggest tools for similar data types
                for data_type in data_types:
                    related_tools = find_tools_by_data_type(data_type, tools_db)
                    for tool in related_tools:
                        if tool['name'] != most_recent and tool not in suggestions:
                            suggestions.append(tool)

                # Get tools commonly used with this one
                related_tools = get_related_tools(most_recent, usage_data, tools_db)

                # Get tools with similar functionality
                similar_tools = find_similar_tools(most_recent, tools_db)

                return {
                    'based_on': most_recent,
                    'handled_data_types': data_types,
                    'suggested_tools': suggestions[:5],
                    'commonly_used_with': related_tools,
                    'similar_tools': similar_tools
                }

    # If no data available or no recent commands, suggest popular starred tools
    popular_tools = []
    for tool_name, tool_info in tools_db.items():
        if 'stars' in tool_info and tool_info['stars'] > 0:
            popular_tools.append({
                'name': tool_name,
                'description': tool_info.get('description', ''),
                'stars': tool_info.get('stars', 0)
            })

    # Sort by stars and take top 5
    popular_tools.sort(key=lambda x: x.get('stars', 0), reverse=True)
    return {
        'popular_tools': popular_tools[:5]
    }

# Use TF-IDF to suggest tools for a specific task


def suggest_tools_for_task(task_description, tools_db):
    """Use TF-IDF or LLM to suggest tools for a specific task"""
    # If LLM is available, use it for better suggestions
    if LLM_AVAILABLE:
        return suggest_tools_with_llm(task_description, tools_db)

    # Otherwise, use TF-IDF matching
    vectorizer = TfidfVectorizer(stop_words='english')

    # Create document corpus with tool descriptions
    documents = []
    tool_names = []
    for tool_name, tool_info in tools_db.items():
        doc = f"{tool_name} {tool_info.get('description', '')} "
        doc += ' '.join(tool_info.get('data_types', []))
        doc += f" {tool_info.get('category', '')}"
        documents.append(doc)
        tool_names.append(tool_name)

    # Add the query
    documents.append(task_description)

    # Vectorize
    tfidf_matrix = vectorizer.fit_transform(documents)

    # Calculate similarity between query and all tools
    query_idx = len(documents) - 1
    query_vector = tfidf_matrix[query_idx:query_idx + 1]
    tool_vectors = tfidf_matrix[:query_idx]

    similarities = cosine_similarity(query_vector, tool_vectors)[0]

    # Get top 5 most similar tools
    top_indices = similarities.argsort()[-5:][::-1]

    results = []
    for idx in top_indices:
        tool_name = tool_names[idx]
        results.append({
            'name': tool_name,
            'description': tools_db[tool_name].get('description', ''),
            'category': tools_db[tool_name].get('category', 'unknown'),
            'data_types': tools_db[tool_name].get('data_types', []),
            'score': float(similarities[idx])
        })

    return {
        'query': task_description,
        'suggested_tools': results
    }

# Use LLM to suggest tools for a specific task


def suggest_tools_with_llm(task_description, tools_db):
    """Use local LLM to suggest tools for a specific task"""
    # Find LLM model in models directory
    model_files = [f for f in os.listdir(MODELS_DIR) if f.endswith('.gguf')]
    if not model_files:
        print("No LLM model found. Please download a model to ~/models/osint/")
        print("Falling back to TF-IDF matching")
        return suggest_tools_for_task(task_description, tools_db)

    model_path = os.path.join(MODELS_DIR, model_files[0])

    # Create a list of tools with descriptions
    tools_text = ""
    for tool_name, tool_info in tools_db.items():
        tools_text += f"- {tool_name}: {tool_info.get('description', '')}\n"
        tools_text += f"  Data types: {', '.join(tool_info.get('data_types', []))}\n"
        tools_text += f"  Category: {tool_info.get('category', 'unknown')}\n"

    # Create a prompt for the LLM
    prompt = f"""You are an OSINT expert. Based on the following task description, suggest which OSINT tools would be most relevant.

Task: {task_description}

Available OSINT tools:
{tools_text}

Suggest the top 5 most relevant tools for this task, and briefly explain why each is useful:"""

    try:
        # Load the model
        llm = Llama(model_path=model_path, n_ctx=2048, verbose=False)

        # Generate suggestion
        output = llm(prompt, max_tokens=1024, temperature=0.1, top_p=0.9, top_k=40, repeat_penalty=1.1)

        # Extract suggested tools from the response
        response_text = output['choices'][0]['text']

        # Process response to extract tool names
        suggested_tools = []
        for tool_name in tools_db.keys():
            if tool_name in response_text:
                # Find the explanation for this tool
                pattern = f"{tool_name}[:\\s]+(.*?)(?=\n\n|\n[A-Za-z0-9]|\\Z)"
                match = re.search(pattern, response_text, re.DOTALL)
                explanation = match.group(1).strip() if match else "Relevant for this task"

                suggested_tools.append({
                    'name': tool_name,
                    'description': tools_db[tool_name].get('description', ''),
                    'category': tools_db[tool_name].get('category', 'unknown'),
                    'data_types': tools_db[tool_name].get('data_types', []),
                    'explanation': explanation
                })

                if len(suggested_tools) >= 5:
                    break

        return {
            'query': task_description,
            'suggested_tools': suggested_tools,
            'llm_response': response_text
        }

    except Exception as e:
        print(f"Error using LLM: {e}")
        print("Falling back to TF-IDF matching")
        return suggest_tools_for_task(task_description, tools_db)

# Generate an OSINT workflow for a specific investigation type


def generate_workflow(investigation_type):
    """Generate an OSINT workflow for a specific investigation type"""
    # Check if we have a predefined workflow
    normalized_type = investigation_type.lower().replace(' ', '_')

    # Load tools database
    tools_db = load_tool_database()

    # If we have a predefined workflow, fill in appropriate tools
    for workflow_key, workflow_template in OSINT_WORKFLOWS.items():
        if normalized_type in workflow_key or workflow_key in normalized_type:
            # Find appropriate tools for each step based on the purpose
            workflow = []
            for step in workflow_template:
                purpose = step["purpose"].lower()

                # Try to find a suitable tool for this purpose
                suitable_tool = None

                # Look for keywords in the purpose
                for tool_name, tool_info in tools_db.items():
                    description = tool_info.get('description', '').lower()

                    # Check if the purpose keywords match the tool description
                    if any(keyword in description for keyword in purpose.split()):
                        suitable_tool = tool_name
                        break

                # If no tool found, leave it as a suggestion
                if suitable_tool:
                    workflow.append({
                        "step": step["step"],
                        "tool": suitable_tool,
                        "purpose": step["purpose"]
                    })
                else:
                    workflow.append({
                        "step": step["step"],
                        "tool": "?",
                        "purpose": step["purpose"],
                        "note": "No suitable tool found in your repository collection"
                    })

            return {
                'investigation_type': investigation_type,
                'workflow': workflow,
                'note': "Tools selected from your GitHub starred repositories. Replace '?' with appropriate tools."
            }

    # If no predefined workflow, try to generate one with LLM if available
    if LLM_AVAILABLE:
        return generate_workflow_with_llm(investigation_type)

    # Otherwise, suggest a generic workflow
    return {
        'investigation_type': investigation_type,
        'workflow': [
            {"step": 1, "tool": "?", "purpose": "Perform initial reconnaissance"},
            {"step": 2, "tool": "?", "purpose": "Gather emails and related information"},
            {"step": 3, "tool": "?", "purpose": "Find social media accounts if applicable"},
            {"step": 4, "tool": "?", "purpose": "Map relationships between discovered entities"},
            {"step": 5, "tool": "?", "purpose": "Discover subdomains if investigating an organization"}
        ],
        'note': "Fill in '?' with tools from your collection. For better results, install llama-cpp-python."
    }

# Use LLM to generate a workflow for a specific investigation type


def generate_workflow_with_llm(investigation_type):
    """Use local LLM to generate an OSINT workflow"""
    # Find LLM model in models directory
    model_files = [f for f in os.listdir(MODELS_DIR) if f.endswith('.gguf')]
    if not model_files:
        print("No LLM model found. Please download a model to ~/models/osint/")
        print("Falling back to generic workflow")
        return generate_workflow(investigation_type)

    model_path = os.path.join(MODELS_DIR, model_files[0])

    # Load tool database
    tools_db = load_tool_database()

    # Create a list of tools with descriptions
    tools_text = ""
    for tool_name, tool_info in tools_db.items():
        tools_text += f"- {tool_name}: {tool_info.get('description', '')}\n"
        tools_text += f"  Data types: {', '.join(tool_info.get('data_types', []))}\n"

    # Create a prompt for the LLM
    prompt = f"""You are an OSINT expert. Design a step-by-step workflow for the following investigation type, using available OSINT tools.

Investigation Type: {investigation_type}

Available OSINT tools:
{tools_text}

Design a workflow with 5-7 steps. For each step, include:
1. The tool to use
2. The purpose of using this tool
3. How it builds on previous steps

Format your response as a numbered list with tool names in bold:"""

    try:
        # Load the model
        llm = Llama(model_path=model_path, n_ctx=2048, verbose=False)

        # Generate workflow
        output = llm(prompt, max_tokens=1024, temperature=0.2, top_p=0.9, top_k=40, repeat_penalty=1.1)

        # Extract workflow from the response
        response_text = output['choices'][0]['text']

        # Process response to extract steps
        workflow = []
        step_pattern = r"(\d+)\.\s+\*\*([^*]+)\*\*:?\s+([^\n]+)"
        matches = re.findall(step_pattern, response_text, re.DOTALL)

        for step_num, tool, purpose in matches:
            # Clean up the tool name and purpose
            tool = tool.strip()
            purpose = purpose.strip()

            # Ensure the tool exists in our database
            if tool in tools_db:
                workflow.append({
                    "step": int(step_num),
                    "tool": tool,
                    "purpose": purpose
                })
            else:
                workflow.append({
                    "step": int(step_num),
                    "tool": "?",
                    "purpose": purpose,
                    "note": f"Suggested {tool} but not found in your repository collection"
                })

        # If we couldn't extract a proper workflow, fall back to a generic one
        if len(workflow) < 3:
            print("Failed to extract workflow from LLM response. Using generic workflow.")
            return generate_workflow(investigation_type)

        return {
            'investigation_type': investigation_type,
            'workflow': workflow,
            'note': "Tools selected from your GitHub starred repositories. Replace '?' with appropriate tools."
        }

    except Exception as e:
        print(f"Error using LLM: {e}")
        print("Falling back to generic workflow")
        return generate_workflow(investigation_type)

# Display statistics about OSINT tool usage


def display_stats():
    """Display statistics about OSINT tool usage"""
    usage_data = parse_usage_log()
    stats = calculate_stats(usage_data)

    print(f"\n=== OSINT Tool Usage Statistics ===")
    print("Total uses: {}".format(stats['total_uses']))
    print("Unique tools: {}".format(stats['unique_tools']))

    print("\nMost used tools:")
    for tool, count in stats['most_used']:
        print(f"  {tool}: {count} uses")

    print("\nRecently used tools:")
    for tool in stats['recent_tools']:
        print(f"  {tool}")

    print("\nUsage by category:")
    for category, count in stats['usage_by_category'].items():
        print(f"  {category}: {count} uses")

    # Daily usage pattern
    if stats['usage_by_day']:
        print("\nDaily usage pattern (last 7 days):")
        sorted_days = sorted(stats['usage_by_day'].keys(), reverse=True)[:7]
        for day in sorted_days:
            count = stats['usage_by_day'][day]
            print(f"  {day}: {count} uses")

# Display search results for tools handling a specific data type


def display_search_results(data_type):
    """Display tools that can handle a specific data type"""
    tools_db = load_tool_database()
    matching_tools = find_tools_by_data_type(data_type, tools_db)

    print(f"\n=== OSINT Tools for {data_type} ===")
    if not matching_tools:
        print('No tools found that handle this data type.')
        return

    for tool in matching_tools:
        print(f"\n{tool['name']}")
        print(f"  Description: {tool['description']}")
        print(f"  Category: {tool['category']}")
        if tool['url']:
            print(f"  URL: {tool['url']}")

# Display tool suggestions


def display_suggestions(query=None):
    """Display suggested OSINT tools"""
    tools_db = load_tool_database()
    usage_data = parse_usage_log()

    suggestions = suggest_tools(query, usage_data, tools_db)

    if query:
        print(f"\n=== Suggested OSINT Tools for: {query} ===")
        if 'suggested_tools' in suggestions:
            for tool in suggestions['suggested_tools']:
                print(f"\n{tool['name']}")
                print(f"  Description: {tool['description']}")
                print(f"  Category: {tool['category']}")
                if 'data_types' in tool:
                    print(f"  Data types: {', '.join(tool['data_types'])}")
                if 'explanation' in tool:
                    print(f"  Why it's useful: {tool['explanation']}")
                elif 'score' in tool:
                    print(f"  Relevance score: {tool['score']:.2f}")
    else:
        if 'based_on' in suggestions:
            print(f"\n=== Tool suggestions based on your recent use of {suggestions['based_on']} ===")
            print(f"This tool handles: {', '.join(suggestions['handled_data_types'])}")

            print("\nSuggested tools for similar data types:")
            for tool in suggestions['suggested_tools']:
                print(f"  {tool['name']}: {tool['description']}")

            print("\nTools commonly used together:")
            for tool, count in suggestions['commonly_used_with']:
                print(f"  {tool}: used together {count} times")

            if 'similar_tools' in suggestions:
                print("\nSimilar tools based on functionality:")
                for tool in suggestions['similar_tools']:
                    print(f"  {tool['name']}: {tool['description']}")
                    print(f"    Similarity: {tool['similarity']:.2f}")
        elif 'popular_tools' in suggestions:
            print("\n=== Popular Tools from Your GitHub Stars ===")
            for tool in suggestions['popular_tools']:
                print(f"  {tool['name']}: {tool['description']}")
                if 'stars' in tool:
                    print(f"    Stars: {tool['stars']}")

# Display an OSINT workflow


def display_workflow(investigation_type):
    """Display an OSINT workflow for a specific investigation type"""
    workflow_data = generate_workflow(investigation_type)

    print(f"\n=== OSINT Workflow for {workflow_data['investigation_type']} ===")

    for step in workflow_data['workflow']:
        print(f"\n{step['step']}. Use {step['tool']}")
        print(f"   Purpose: {step['purpose']}")
        if 'note' in step:
            print(f"   Note: {step['note']}")

    if 'note' in workflow_data:
        print(f"\nNote: {workflow_data['note']}")

# Build or update the OSINT tools database


def build_database():
    """Build or update the OSINT tools database from GitHub stars and installed tools"""
    print("Building OSINT tool database from your GitHub starred repositories...")

    # Start with tools extracted from starred repos
    tool_db = extract_tools_from_starred_repos()

    # Also check if any tools are installed but not in our database
    print("Checking for installed OSINT tools...")

    # Common locations for tools
    paths = os.environ['PATH'].split(':')

    detected_tools = []
    for path in paths:
        if os.path.exists(path):
            for item in os.listdir(path):
                # Check against known tools that might not be in our predefined list
                file_path = os.path.join(path, item)
                if os.path.isfile(file_path) and os.access(file_path, os.X_OK):
                    # Common OSINT tool name patterns
                    tool_patterns = [
                        "osint", "recon", "harvest", "scan", "enum", "find", "search",
                        "info", "gather", "dork", "whois", "dig", "nslookup", "maltego",
                        "spoof", "metasploit", "burp", "nikto", "nmap", "masscan"
                    ]

                    if any(pattern in item.lower() for pattern in tool_patterns) and item not in tool_db:
                        detected_tools.append(item)

    if detected_tools:
        print(f"Detected {len(detected_tools)} potential OSINT tools not in database.")

        # For each detected tool, try to get more information
        for tool in detected_tools:
            # Try to get help text
            try:
                help_text = os.popen(f"{tool} --help 2>&1").read()
                if not help_text:
                    help_text = os.popen(f"{tool} -h 2>&1").read()

                # Extract description from help text (first line often contains it)
                description = help_text.split('\n')[0] if help_text else "Unknown OSINT tool"

                # Make a guess at data types based on help text
                data_types = []
                for data_type in ["email", "domain", "ip", "username", "phone", "social", "password", "hash"]:
                    if data_type in help_text.lower():
                        data_types.append(data_type)

                if not data_types:
                    data_types = ["unknown"]

                # Add to tool database
                tool_db[tool] = {
                    "description": description,
                    "data_types": data_types,
                    "category": "unknown",
                    "url": "",
                    "source": "installed"
                }

                print(f"Added {tool} to database with {len(data_types)} data types")
            except Exception as e:
                print(f"Error analyzing {tool}: {e}")

    # Save the updated database
    with open(TOOL_DB_PATH, 'w') as f:
        json.dump(tool_db, f, indent=4)

    print(f"OSINT tool database built with {len(tool_db)} tools.")
    return tool_db

# Main function


def main():
    """Main function for the OSINT script"""
    # Create directories if they don't exist
    os.makedirs(OSINT_DIR, exist_ok=True)
    os.makedirs(HISTORY_DIR, exist_ok=True)
    os.makedirs(REPOS_DIR, exist_ok=True)
    os.makedirs(MODELS_DIR, exist_ok=True)
    os.makedirs(CACHE_DIR, exist_ok=True)

    # Parse arguments
    parser = argparse.ArgumentParser(description='VANTAGE OSINT Intelligence Tool')
    parser.add_argument(
        '--suggest',
        nargs='?',
        const=None,
        help='Suggest OSINT tools based on usage or for a specific task')
    parser.add_argument('--search', help='Search for OSINT tools handling a specific data type')
    parser.add_argument('--build', action='store_true', help='Build or update the OSINT tool database')
    parser.add_argument('--stats', action='store_true', help='Show statistics about OSINT tool usage')
    parser.add_argument('--workflow', help='Generate an OSINT workflow for a specific investigation type')

    args = parser.parse_args()

    if args.build:
        build_database()
    elif args.stats:
        display_stats()
    elif args.search:
        display_search_results(args.search)
    elif args.suggest is not None:
        display_suggestions(args.suggest)
    elif args.workflow:
        display_workflow(args.workflow)
    else:
        parser.print_help()


if __name__ == '__main__':
    main()
