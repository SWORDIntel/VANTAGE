#!/usr/bin/env python3
"""
Auto-sort script for VANTAGE repository
Automatically sorts remaining files in UNSORTED directory based on keyword matching
"""

import os
import re
import shutil
import json
from pathlib import Path
import argparse

# Define category keywords for classification
CATEGORY_KEYWORDS = {
    "malware": [
        "malware", "exploit", "backdoor", "rootkit", "ransomware", "payload", "shellcode", 
        "trojan", "dropper", "botnet", "keylogger", "spyware", "worm", "stealer", 
        "lateral movement", "c2", "command and control", "persistence", "evasion", 
        "offensive", "pentest", "penetration testing", "red team", "privilege escalation",
        "obfuscation", "injection", "token", "mimikatz", "lsass", "credential", "dumping",
        "shellcode", "memory", "hook", "edr", "av bypass", "defender", "unhook"
    ],
    
    "malware-rats": [
        "rat", "remote access", "remote administration", "remote admin tool", "c2 framework",
        "command and control framework"
    ],

    "network": [
        "network", "protocol", "packet", "sniffer", "capture", "pcap", "traffic", "analysis", 
        "ethernet", "routing", "router", "switch", "firewall", "ips", "ids", "flow", "netflow",
        "scada", "modbus", "ics", "industrial", "plc", "sctp", "ss7", "gsm", "lte", "5g",
        "telecom", "voip", "sip", "diameter", "vpn", "tunneling", "dns", "dhcp", "ntp",
        "snmp", "icmp", "tcp", "udp", "ipv4", "ipv6", "wireless", "bluetooth", "zigbee",
        "cellular", "mobile", "scanning", "recon", "reconnaissance", "enumeration"
    ],
    
    "osint": [
        "osint", "recon", "reconnaissance", "intelligence", "collection", "scraping", 
        "gathering", "crawler", "search", "discovery", "investigation", "dork", "shodan", 
        "censys", "zoomeye", "google dork", "information gathering", "social media", 
        "metadata", "geolocation", "geoip", "email", "phone", "username", "identity",
        "lookup", "whois", "dns", "subdomain", "passive", "leaks", "breached", "dark web",
        "source", "tracking", "footprinting", "enumeration", "domain"
    ],
    
    "ai": [
        "ai", "machine learning", "ml", "deep learning", "dl", "neural", "transformer", 
        "gpt", "bert", "llm", "llama", "model", "training", "dataset", "prompt", "embedding", 
        "vector", "inference", "classification", "clustering", "nlp", "natural language", 
        "computer vision", "cv", "reinforcement", "generative", "diffusion"
    ],
    
    "proxy": [
        "proxy", "forward proxy", "reverse proxy", "socks", "http proxy", "tunneling", 
        "nat traversal", "hole punching", "relay", "load balancer", "gateway", "vpn",
        "tunnel", "frp", "ngrok", "cloudflare"
    ],
    
    "telegram": [
        "telegram", "bot", "tg", "message", "channel", "group", "mtproto"
    ],
    
    "community": [
        "community", "discord", "forum", "social", "messaging", "collaboration", "chat",
        "feedback", "contribution", "moderation", "plugin", "extension", "addon"
    ],
    
    "productivity": [
        "productivity", "workflow", "automation", "scheduler", "reminder", "todo", "kanban", 
        "project management", "time tracking", "notes", "knowledge base", "documentation", 
        "calendar", "organization", "efficiency", "utility", "tool"
    ],
    
    "wireless": [
        "wireless", "wifi", "802.11", "bluetooth", "zigbee", "z-wave", "rf", "radio", 
        "spectrum", "scanner", "jamming", "deauthentication", "beacon", "handshake", 
        "wpa", "wep", "wps", "signal", "antenna", "firmware", "driver", "sdr", 
        "software defined radio"
    ],
    
    "terminalai": [
        "terminal", "cli", "command line", "tui", "text user interface", "console", 
        "shell", "bash", "zsh", "prompt", "curses", "completion", "autocomplete"
    ]
}

# Try to load additional keywords from auto_sort_keywords.json if it exists
try:
    keywords_file = Path('auto_sort_keywords.json')
    if keywords_file.exists():
        with open(keywords_file, 'r', encoding='utf-8') as f:
            additional_keywords = json.load(f)
            
        # Merge with existing keywords
        for category, words in additional_keywords.items():
            if category in CATEGORY_KEYWORDS:
                # Add only keywords that don't already exist
                existing_words = set(CATEGORY_KEYWORDS[category])
                CATEGORY_KEYWORDS[category].extend([w for w in words if w not in existing_words])
            else:
                CATEGORY_KEYWORDS[category] = words
                
        print(f"Loaded additional keywords from {keywords_file}")
except Exception as e:
    print(f"Warning: Could not load additional keywords: {e}")

def read_file_content(file_path):
    """Read file content safely, handling encoding issues."""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            return f.read()
    except UnicodeDecodeError:
        try:
            with open(file_path, 'r', encoding='latin-1') as f:
                return f.read()
        except Exception as e:
            print(f"Error reading {file_path}: {e}")
            return ""

def classify_file(file_path):
    """Classify a file based on its content and keywords."""
    content = read_file_content(file_path)
    
    # Count matches for each category
    scores = {category: 0 for category in CATEGORY_KEYWORDS}
    
    for category, keywords in CATEGORY_KEYWORDS.items():
        for keyword in keywords:
            # Case insensitive search
            pattern = re.compile(r'\b' + re.escape(keyword) + r'\b', re.IGNORECASE)
            matches = pattern.findall(content)
            scores[category] += len(matches)
    
    # Find the category with the highest score
    best_category = max(scores.items(), key=lambda x: x[1])
    
    # If no matches, return None
    if best_category[1] == 0:
        return None, 0
    
    return best_category[0], best_category[1]

def move_file(file_path, destination_dir):
    """Move a file to the destination directory."""
    try:
        # Ensure destination directory exists
        os.makedirs(destination_dir, exist_ok=True)
        
        # Move the file
        shutil.move(str(file_path), destination_dir)
        print(f"Moved {file_path.name} to {destination_dir}")
        return True
    except Exception as e:
        print(f"Error moving {file_path}: {e}")
        return False

def main():
    """Main function to sort files."""
    parser = argparse.ArgumentParser(description='Auto-sort files from UNSORTED directory based on keywords.')
    parser.add_argument('--dry-run', action='store_true', help='Dry run without moving files')
    parser.add_argument('--min-score', type=int, default=2, help='Minimum score to classify a file (default: 2)')
    parser.add_argument('--unsorted-dir', default='gitstar/readmes/UNSORTED', help='Path to UNSORTED directory')
    parser.add_argument('--dest-base', default='gitstar/readmes', help='Base directory for categories')
    
    args = parser.parse_args()
    
    # Get all markdown files from UNSORTED directory
    unsorted_dir = Path(args.unsorted_dir)
    all_files = list(unsorted_dir.glob('*.md'))
    
    # Skip progress.md
    files_to_process = [f for f in all_files if f.name != 'progress.md']
    
    print(f"Found {len(files_to_process)} files to process")
    
    # Classification results
    results = {
        'sorted': [],
        'unclassified': [],
        'low_score': []
    }
    
    for file_path in files_to_process:
        category, score = classify_file(file_path)
        
        if category is None:
            results['unclassified'].append((file_path.name, None, 0))
            print(f"Unclassified: {file_path.name}")
            continue
        
        if score < args.min_score:
            results['low_score'].append((file_path.name, category, score))
            print(f"Low score: {file_path.name} -> {category} (score: {score})")
            continue
        
        dest_dir = Path(args.dest_base) / category
        
        if not args.dry_run:
            if move_file(file_path, dest_dir):
                results['sorted'].append((file_path.name, category, score))
        else:
            print(f"Would move {file_path.name} to {category} (score: {score})")
            results['sorted'].append((file_path.name, category, score))
    
    # Print summary
    print("\n--- Summary ---")
    print(f"Sorted: {len(results['sorted'])}")
    print(f"Low score: {len(results['low_score'])}")
    print(f"Unclassified: {len(results['unclassified'])}")
    
    # Print detailed results
    if results['sorted']:
        print("\n--- Sorted Files ---")
        for name, category, score in results['sorted']:
            print(f"{name} -> {category} (score: {score})")
    
    if results['low_score']:
        print("\n--- Low Score Files ---")
        for name, category, score in results['low_score']:
            print(f"{name} -> {category} (score: {score})")
    
    if results['unclassified']:
        print("\n--- Unclassified Files ---")
        for name, _, _ in results['unclassified']:
            print(name)

if __name__ == "__main__":
    main() 