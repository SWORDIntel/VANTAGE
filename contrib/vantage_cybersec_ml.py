#!/usr/bin/env python3
# vantage_cybersec_ml.py: Advanced cybersecurity machine learning module for VANTAGE
# Builds on GitHub Star Analyzer to add specialized cybersecurity analysis capabilities

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
import random
from pathlib import Path
import importlib.util
from datetime import datetime
import logging
import numpy as np

# Third-party imports (with robust error handling)
try:
    import requests
    from tqdm import tqdm
    import joblib
    DEPENDENCIES_MET = True
except ImportError as e:
    logging.error(f"Missing dependency: {e}")
    logging.error("Install requirements with: pip install requests tqdm numpy joblib scikit-learn scipy tensorflow")
    DEPENDENCIES_MET = False

ML_AVAILABLE = False
try:
    from sklearn.feature_extraction.text import TfidfVectorizer
    from sklearn.ensemble import RandomForestClassifier, IsolationForest
    from sklearn.cluster import DBSCAN
    from sklearn.metrics import classification_report
    from sklearn.model_selection import train_test_split
    ML_AVAILABLE = True
except ImportError as e:
    logging.error(f"Machine learning libraries not available: {e}")
    logging.error("Install with: pip install scikit-learn scipy")

ADV_ML_AVAILABLE = False
try:
    from tensorflow.keras.models import load_model
    ADV_ML_AVAILABLE = True
except ImportError as e:
    logging.warning(f"Advanced ML libraries not available: {e}")
    logging.warning("Install with: pip install tensorflow")

LLM_AVAILABLE = False
try:
    from llama_cpp import Llama
    LLM_AVAILABLE = True
except ImportError:
    logging.warning("llama-cpp-python not available, advanced analysis will be limited")
    logging.warning("Install with: pip install llama-cpp-python")

# Try to import the context module and gitstar analyzer for integration
CONTEXT_AVAILABLE = False
GITSTAR_AVAILABLE = False
context_module = None
gitstar_module = None

VANTAGE_DIR = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
CONTEXT_MODULE_PATH = os.path.join(VANTAGE_DIR, "contrib", "vantage_context.py")
GITSTAR_MODULE_PATH = os.path.join(VANTAGE_DIR, "contrib", "vantage_gitstar.py")

if os.path.exists(CONTEXT_MODULE_PATH):
    try:
        spec = importlib.util.spec_from_file_location("vantage_context", CONTEXT_MODULE_PATH)
        context_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(context_module)
        CONTEXT_AVAILABLE = True
        logging.info("Context module loaded successfully")
    except Exception as e:
        logging.error(f"Error loading vantage_context module: {e}")
        CONTEXT_AVAILABLE = False

if os.path.exists(GITSTAR_MODULE_PATH):
    try:
        spec = importlib.util.spec_from_file_location("vantage_gitstar", GITSTAR_MODULE_PATH)
        gitstar_module = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(gitstar_module)
        GITSTAR_AVAILABLE = True
        logging.info("GitStar module loaded successfully")
    except Exception as e:
        logging.error(f"Error loading vantage_gitstar module: {e}")
        GITSTAR_AVAILABLE = False

# Constants
HOME_DIR = os.path.expanduser("~")
CYBERSEC_DIR = os.path.join(HOME_DIR, ".vantage", "cybersec")
DATASETS_DIR = os.path.join(CYBERSEC_DIR, "datasets")
MODELS_DIR = os.path.join(CYBERSEC_DIR, "models")
SCAN_RESULTS_DIR = os.path.join(CYBERSEC_DIR, "scan_results")
CONFIG_FILE = os.path.join(CYBERSEC_DIR, "config.json")
DEFAULT_MODEL_PATH = os.path.join(HOME_DIR, ".vantage", "models", "llama-2-7b-chat.Q4_K_M.gguf")
VULN_DB_PATH = os.path.join(CYBERSEC_DIR, "vulnerability_db.json")
SIGNATURE_DB_PATH = os.path.join(CYBERSEC_DIR, "signatures.json")

GLOVE_PATH = os.path.expanduser("~/.vantage/models/glove.6B.100d.txt")  # Default path for GloVe 100d
GLOVE_DIM = 100

# Ensure directories exist
Path(CYBERSEC_DIR).mkdir(parents=True, exist_ok=True)
Path(DATASETS_DIR).mkdir(parents=True, exist_ok=True)
Path(MODELS_DIR).mkdir(parents=True, exist_ok=True)
Path(SCAN_RESULTS_DIR).mkdir(parents=True, exist_ok=True)

# Default configuration
DEFAULT_CONFIG = {
    "detection_threshold": 0.75,
    "analyze_libraries": True,
    "max_file_size": 1024 * 1024 * 10,  # 10MB
    "ignored_directories": [".git", "node_modules", "__pycache__", "venv", ".env"],
    "last_update_check": 0,
    "update_frequency": 7 * 24 * 60 * 60,  # 1 week in seconds
    "api_endpoints": {
        "nvd": "https://services.nvd.nist.gov/rest/json/cves/2.0",
        "github_advisory": "https://api.github.com/advisories"
    },
    "llm_settings": {
        "temperature": 0.1,
        "max_tokens": 1024,
        "top_p": 0.9,
        "top_k": 40,
        "context_size": 4096
    }
}


class CybersecurityMLAnalyzer:
    """Advanced machine learning analyzer for cybersecurity applications"""

    def __init__(self, config=None):
        self.config = self._load_config()
        self.vulnerability_db = self._load_vulnerability_db()
        self.signature_db = self._load_signature_db()
        self.models = {}
        self.llm = None
        self.feature_type = "glove"  # Default to GloVe if available
        if config and "feature_type" in config:
            self.feature_type = config["feature_type"].lower()
        self.glove_embeddings = None
        self.glove_dim = GLOVE_DIM
        self._load_glove_embeddings()

        # Initialize ML components if available
        if ML_AVAILABLE:
            self._initialize_ml()

        # Initialize advanced ML if available
        if ADV_ML_AVAILABLE:
            self._initialize_advanced_ml()

        # Initialize LLM if available
        if LLM_AVAILABLE:
            self._initialize_llm()

    def _load_config(self):
        """Load configuration from JSON file or create default"""
        if os.path.exists(CONFIG_FILE):
            try:
                with open(CONFIG_FILE, "r") as f:
                    config = json.load(f)
                    # Update with any missing defaults
                    for key, value in DEFAULT_CONFIG.items():
                        if key not in config:
                            config[key] = value
                    return config
            except json.JSONDecodeError:
                logging.warning("Invalid config file, using defaults")
                return DEFAULT_CONFIG.copy()

        # Create default config
        with open(CONFIG_FILE, "w") as f:
            json.dump(DEFAULT_CONFIG, f, indent=2)
        return DEFAULT_CONFIG.copy()

    def _load_vulnerability_db(self):
        """Load vulnerability database"""
        if os.path.exists(VULN_DB_PATH):
            try:
                with open(VULN_DB_PATH, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                logging.warning("Invalid vulnerability database, creating new one")
                return {"vulnerabilities": [], "last_updated": 0}
        return {"vulnerabilities": [], "last_updated": 0}

    def _load_signature_db(self):
        """Load signature database for pattern matching"""
        if os.path.exists(SIGNATURE_DB_PATH):
            try:
                with open(SIGNATURE_DB_PATH, "r") as f:
                    return json.load(f)
            except json.JSONDecodeError:
                logging.warning("Invalid signature database, creating new one")
                return {"signatures": [], "last_updated": 0}
        return {"signatures": [], "last_updated": 0}

    def _load_glove_embeddings(self):
        """Load GloVe embeddings if available and selected."""
        if self.feature_type != "glove":
            return
        if not os.path.exists(GLOVE_PATH):
            logging.warning(f"GloVe embeddings not found at {GLOVE_PATH}. Falling back to TF-IDF.")
            self.feature_type = "tfidf"
            return
        self.glove_embeddings = {}
        try:
            with open(GLOVE_PATH, 'r', encoding='utf-8') as f:
                for line in f:
                    parts = line.strip().split()
                    word = parts[0]
                    vector = np.asarray(parts[1:], dtype='float32')
                    if len(vector) == self.glove_dim:
                        self.glove_embeddings[word] = vector
            logging.info(f"Loaded GloVe embeddings from {GLOVE_PATH}")
        except Exception as e:
            logging.error(f"Error loading GloVe embeddings: {e}")
            self.feature_type = "tfidf"

    def _text_to_embedding(self, text):
        """Convert text to an average GloVe embedding vector."""
        if not self.glove_embeddings:
            return np.zeros(self.glove_dim)
        words = text.split()
        vectors = [self.glove_embeddings[w] for w in words if w in self.glove_embeddings]
        if not vectors:
            return np.zeros(self.glove_dim)
        return np.mean(vectors, axis=0)

    def _vectorize_corpus(self, texts, fit=False):
        """Vectorize a list of texts using the selected feature type."""
        if self.feature_type == "glove" and self.glove_embeddings:
            return np.array([self._text_to_embedding(txt) for txt in texts])
        # Fallback to TF-IDF
        if fit or not hasattr(self, 'tfidf_vectorizer'):
            self.tfidf_vectorizer = TfidfVectorizer(
                max_features=10000,
                ngram_range=(1, 3),
                analyzer='word',
                token_pattern=r'(?u)\b\w+\b|[^\w\s]',
                stop_words=None
            )
            return self.tfidf_vectorizer.fit_transform(texts)
        else:
            return self.tfidf_vectorizer.transform(texts)

    def _initialize_ml(self):
        """Initialize basic machine learning components"""
        logging.info("Initializing machine learning components")

        # Load pre-trained models if they exist
        vulncode_model_path = os.path.join(MODELS_DIR, "vulncode_classifier.joblib")
        if os.path.exists(vulncode_model_path):
            try:
                self.models["vulncode_classifier"] = joblib.load(vulncode_model_path)
                logging.info("Loaded vulnerability code classifier model")
            except Exception as e:
                logging.error(f"Error loading vulnerability code classifier: {e}")

        anomaly_model_path = os.path.join(MODELS_DIR, "anomaly_detector.joblib")
        if os.path.exists(anomaly_model_path):
            try:
                self.models["anomaly_detector"] = joblib.load(anomaly_model_path)
                logging.info("Loaded anomaly detection model")
            except Exception as e:
                logging.error(f"Error loading anomaly detection model: {e}")

        # Initialize text vectorizers for code analysis
        if self.feature_type == "tfidf":
            self.tfidf_vectorizer = TfidfVectorizer(
                max_features=10000,
                ngram_range=(1, 3),
                analyzer='word',
                token_pattern=r'(?u)\b\w+\b|[^\w\s]',
                stop_words=None
            )

    def _initialize_advanced_ml(self):
        """Initialize advanced ML components (TensorFlow based)"""
        logging.info("Initializing advanced machine learning components")

        # Load deep learning models if they exist
        dl_model_path = os.path.join(MODELS_DIR, "deep_vulncode_model.h5")
        if os.path.exists(dl_model_path):
            try:
                self.models["deep_vulncode"] = load_model(dl_model_path)
                logging.info("Loaded deep learning vulnerability model")
            except Exception as e:
                logging.error(f"Error loading deep learning model: {e}")

    def _initialize_llm(self):
        """Initialize the LLM for advanced analysis"""
        logging.info("Initializing LLM")

        model_path = self.config.get("model_path", DEFAULT_MODEL_PATH)
        if os.path.exists(model_path):
            try:
                llm_settings = self.config.get("llm_settings", {})
                self.llm = Llama(
                    model_path=model_path,
                    n_ctx=llm_settings.get("context_size", 4096),
                    n_threads=os.cpu_count() or 4,
                    verbose=False
                )
                logging.info(f"LLM initialized with {model_path}")
            except Exception as e:
                logging.error(f"Error initializing LLM: {e}")
                self.llm = None
        else:
            logging.warning(f"Model file not found: {model_path}")

    def save_config(self):
        """Save configuration to file"""
        with open(CONFIG_FILE, "w") as f:
            json.dump(self.config, f, indent=2)

    def save_vulnerability_db(self):
        """Save vulnerability database to file"""
        with open(VULN_DB_PATH, "w") as f:
            json.dump(self.vulnerability_db, f, indent=2)

    def save_signature_db(self):
        """Save signature database to file"""
        with open(SIGNATURE_DB_PATH, "w") as f:
            json.dump(self.signature_db, f, indent=2)

    def _generate_secure_token(self, data, key=None):
        """Generate a secure HMAC token for data verification"""
        if key is None:
            key = os.urandom(32)

        # Create HMAC signature
        h = hmac.new(key, data.encode('utf-8'), hashlib.sha256)
        signature = h.digest()

        # Combine key and signature for verification
        token = base64.b64encode(key + signature).decode('utf-8')
        return token

    def _verify_secure_token(self, token, data):
        """Verify a secure HMAC token against data"""
        try:
            # Decode the token to get key and signature
            decoded = base64.b64decode(token.encode('utf-8'))
            key, original_signature = decoded[:32], decoded[32:]

            # Recreate the signature with the same key
            h = hmac.new(key, data.encode('utf-8'), hashlib.sha256)
            new_signature = h.digest()

            # Compare signatures with constant-time comparison
            return hmac.compare_digest(original_signature, new_signature)
        except Exception as e:
            logging.error(f"Token verification error: {e}")
            return False

    def update_vulnerability_database(self, force=False):
        """Update vulnerability database from online sources"""
        logging.info("Checking for vulnerability database updates")

        current_time = time.time()
        last_update = self.vulnerability_db.get("last_updated", 0)
        update_frequency = self.config.get("update_frequency", DEFAULT_CONFIG["update_frequency"])

        # Check if update is needed
        if not force and (current_time - last_update) < update_frequency:
            logging.info("Vulnerability database is up to date")
            return False

        try:
            # Update from NVD
            logging.info("Updating from NVD database")
            nvd_endpoint = self.config["api_endpoints"]["nvd"]

            # Get last 30 days of vulnerabilities
            params = {
                "pubStartDate": datetime.fromtimestamp(
                    current_time - 30 * 24 * 60 * 60).strftime("%Y-%m-%dT00:00:00.000"),
                "pubEndDate": datetime.fromtimestamp(current_time).strftime("%Y-%m-%dT23:59:59.999")}

            headers = {"User-Agent": "VANTAGE-Cybersec/1.0"}
            response = requests.get(nvd_endpoint, params=params, headers=headers)

            if response.status_code == 200:
                try:
                    nvd_data = response.json()
                    for vuln in nvd_data.get("vulnerabilities", []):
                        cve = vuln.get("cve", {})

                        # Process and add to our database if it's code-related
                        if self._is_code_vulnerability(cve):
                            self._process_cve_record(cve)

                    logging.info("Added/updated vulnerabilities from NVD")
                except json.JSONDecodeError:
                    logging.error("Invalid JSON response from NVD")
            else:
                logging.error(f"Error fetching from NVD: {response.status_code}")

            # Update last_updated timestamp
            self.vulnerability_db["last_updated"] = current_time
            self.save_vulnerability_db()

            return True

        except Exception as e:
            logging.error(f"Error updating vulnerability database: {e}")
            return False

    def _is_code_vulnerability(self, cve):
        """Check if a CVE record is related to code vulnerabilities"""
        # Look for code-related keywords in the description
        description = ""
        for desc in cve.get("descriptions", []):
            if desc.get("lang") == "en":
                description = desc.get("value", "")
                break

        code_keywords = [
            "code execution", "injection", "xss", "csrf", "overflow",
            "command injection", "sql injection", "deserialization",
            "memory corruption", "path traversal", "race condition"
        ]

        for keyword in code_keywords:
            if keyword.lower() in description.lower():
                return True

        return False

    def _process_cve_record(self, cve):
        """Process a CVE record and add to our database"""
        # Extract relevant information
        cve_id = cve.get("id", "")

        # Get description
        description = ""
        for desc in cve.get("descriptions", []):
            if desc.get("lang") == "en":
                description = desc.get("value", "")
                break

        # Get CVSS score and vector
        metrics = cve.get("metrics", {})
        cvss_v3 = metrics.get("cvssMetricV31", [{}])[0] if "cvssMetricV31" in metrics else {}
        cvss_v3 = cvss_v3.get("cvssData", {}) if cvss_v3 else {}

        cvss_score = cvss_v3.get("baseScore", 0)
        cvss_vector = cvss_v3.get("vectorString", "")

        # Get references
        references = []
        for ref in cve.get("references", []):
            references.append(ref.get("url", ""))

        # Create vulnerability record
        vuln_record = {
            "id": cve_id,
            "description": description,
            "cvss_score": cvss_score,
            "cvss_vector": cvss_vector,
            "references": references,
            "published": cve.get("published", ""),
            "last_modified": cve.get("lastModified", ""),
            "signatures": self._generate_signatures_from_cve(description)
        }

        # Add or update in our database
        existing_vulns = [v for v in self.vulnerability_db["vulnerabilities"] if v["id"] == cve_id]
        if existing_vulns:
            # Update existing entry
            index = self.vulnerability_db["vulnerabilities"].index(existing_vulns[0])
            self.vulnerability_db["vulnerabilities"][index] = vuln_record
        else:
            # Add new entry
            self.vulnerability_db["vulnerabilities"].append(vuln_record)

    def _generate_signatures_from_cve(self, description):
        """Generate code signatures from CVE description using LLM"""
        if not LLM_AVAILABLE or not self.llm:
            return []

        try:
            prompt = f"""
            Based on the following vulnerability description, generate specific code patterns that might indicate this vulnerability.
            Focus on actual code signatures, not general descriptions.

            Vulnerability description:
            {description}

            Return the response as a JSON array of signature objects with these fields:
            - pattern: A regex pattern or specific code sequence that indicates the vulnerability
            - language: The programming language (e.g., 'python', 'javascript', 'c', 'java', etc.)
            - severity: A number from 1-5 indicating severity (5 being most severe)
            - description: Brief description of what this pattern indicates
            - mitigation: How to fix this issue

            Only return valid JSON, nothing else.
            """

            output = self.llm(
                prompt,
                max_tokens=self.config["llm_settings"]["max_tokens"],
                temperature=self.config["llm_settings"]["temperature"],
                top_p=self.config["llm_settings"]["top_p"],
                top_k=self.config["llm_settings"]["top_k"],
                stop=["```"],
                echo=False
            )

            result_text = output["choices"][0]["text"].strip()

            # Extract JSON part
            json_match = re.search(r'\[.*\]', result_text, re.DOTALL)
            if json_match:
                signatures = json.loads(json_match.group(0))
                return signatures

            return []

        except Exception as e:
            logging.error(f"Error generating signatures with LLM: {e}")
            return []

    def scan_codebase(self, directory, recursive=True, file_types=None, excluded_dirs=None):
        """Scan a codebase for potential security vulnerabilities"""
        logging.info(f"Scanning codebase in {directory}")

        if not os.path.exists(directory) or not os.path.isdir(directory):
            logging.error(f"Directory {directory} does not exist")
            return {"success": False, "error": "Directory does not exist"}

        # Use default excluded dirs if not specified
        if excluded_dirs is None:
            excluded_dirs = self.config.get("ignored_directories", DEFAULT_CONFIG["ignored_directories"])

        # Default file types to scan
        if file_types is None:
            file_types = [
                ".py", ".js", ".ts", ".php", ".java", ".rb", ".go", ".c", ".cpp",
                ".h", ".hpp", ".cs", ".sh", ".bash", ".html", ".xml", ".json", ".yml", ".yaml"
            ]

        # Ensure file types start with dot
        file_types = [ft if ft.startswith(".") else "." + ft for ft in file_types]

        # Results container
        scan_result = {
            "timestamp": time.time(),
            "directory": directory,
            "findings": [],
            "statistics": {
                "files_scanned": 0,
                "files_with_issues": 0,
                "total_issues": 0,
                "severity_counts": {1: 0, 2: 0, 3: 0, 4: 0, 5: 0}
            },
            "success": True
        }

        # Get all files to scan
        files_to_scan = []
        try:
            for root, dirs, files in os.walk(directory):
                # Skip excluded directories
                dirs[:] = [d for d in dirs if d not in excluded_dirs]

                for file in files:
                    file_path = os.path.join(root, file)
                    file_ext = os.path.splitext(file)[1].lower()

                    # Skip files that are too large
                    if os.path.getsize(file_path) > self.config.get("max_file_size", DEFAULT_CONFIG["max_file_size"]):
                        logging.warning(f"Skipping {file_path} - exceeds size limit")
                        continue

                    # Check file extension
                    if file_ext in file_types:
                        files_to_scan.append(file_path)

                # Stop walking if not recursive
                if not recursive:
                    break

            logging.info(f"Found {len(files_to_scan)} files to scan")

            # Scan each file
            with tqdm(total=len(files_to_scan), desc="Scanning files") as pbar:
                for file_path in files_to_scan:
                    file_findings = self._scan_single_file(file_path)

                    if file_findings:
                        scan_result["findings"].extend(file_findings)
                        scan_result["statistics"]["files_with_issues"] += 1
                        scan_result["statistics"]["total_issues"] += len(file_findings)

                        # Update severity counts
                        for finding in file_findings:
                            severity = finding.get("severity", 3)
                            scan_result["statistics"]["severity_counts"][severity] = \
                                scan_result["statistics"]["severity_counts"].get(severity, 0) + 1

                    scan_result["statistics"]["files_scanned"] += 1
                    pbar.update(1)

            # Sort findings by severity (highest first)
            scan_result["findings"].sort(key=lambda x: x.get("severity", 3), reverse=True)

            # Save scan results
            timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
            result_file = os.path.join(SCAN_RESULTS_DIR, f"scan_{timestamp}.json")
            with open(result_file, "w") as f:
                json.dump(scan_result, f, indent=2)

            logging.info(
                f"Scan completed. Found {scan_result['statistics']['total_issues']} potential issues in "
                f"{scan_result['statistics']['files_with_issues']} files"
            )
            logging.info(f"Results saved to {result_file}")

            # Add machine learning analysis if available
            if ML_AVAILABLE and len(scan_result["findings"]) > 0:
                self._analyze_scan_with_ml(scan_result, result_file)

            return scan_result

        except Exception as e:
            logging.error(f"Error scanning codebase: {e}")
            return {"success": False, "error": str(e)}

    def _scan_single_file(self, file_path):
        """Scan a single file for vulnerabilities"""
        file_findings = []

        try:
            # Determine file type
            file_ext = os.path.splitext(file_path)[1].lower()
            language = self._get_language_from_extension(file_ext)

            # Read file content
            with open(file_path, "r", encoding="utf-8", errors="ignore") as f:
                content = f.read()

            relative_path = os.path.relpath(file_path)

            # Apply signature-based detection
            signature_findings = self._check_signatures(content, language, relative_path)
            file_findings.extend(signature_findings)

            # Apply ML-based detection if available
            if ML_AVAILABLE and "vulncode_classifier" in self.models:
                ml_findings = self._check_with_ml(content, language, relative_path)
                file_findings.extend(ml_findings)

            # Apply regex-based common vulnerability patterns
            regex_findings = self._check_common_patterns(content, language, relative_path)
            file_findings.extend(regex_findings)

            # Apply advanced analysis with LLM if available and file isn't too long
            if LLM_AVAILABLE and self.llm and len(content) < 8000:
                llm_findings = self._check_with_llm(content, language, relative_path)
                file_findings.extend(llm_findings)

            return file_findings

        except Exception as e:
            logging.error(f"Error scanning {file_path}: {e}")
            return []

    def _get_language_from_extension(self, ext):
        """Map file extension to language name"""
        language_map = {
            ".py": "python",
            ".js": "javascript",
            ".ts": "typescript",
            ".jsx": "javascript",
            ".tsx": "typescript",
            ".php": "php",
            ".java": "java",
            ".rb": "ruby",
            ".go": "go",
            ".c": "c",
            ".cpp": "cpp",
            ".h": "c",
            ".hpp": "cpp",
            ".cs": "csharp",
            ".sh": "bash",
            ".bash": "bash",
            ".html": "html",
            ".xml": "xml",
            ".json": "json",
            ".yml": "yaml",
            ".yaml": "yaml"
        }
        return language_map.get(ext, "unknown")

    def _check_signatures(self, content, language, file_path):
        """Check content against vulnerability signatures"""
        findings = []

        # Get relevant signatures for this language
        relevant_signatures = [
            sig for sig in self.signature_db.get("signatures", [])
            if sig.get("language") == language or sig.get("language") == "any"
        ]

        # Check each signature
        for signature in relevant_signatures:
            pattern = signature.get("pattern", "")
            if not pattern:
                continue

            try:
                matches = re.finditer(pattern, content, re.MULTILINE)
                for match in matches:
                    # Get match line numbers
                    start_line = content[:match.start()].count('\n') + 1
                    end_line = start_line + content[match.start():match.end()].count('\n')
                    matched_text = match.group(0)

                    finding = {
                        "type": "signature_match",
                        "signature_id": signature.get("id", "unknown"),
                        "file": file_path,
                        "language": language,
                        "start_line": start_line,
                        "end_line": end_line,
                        "description": signature.get("description", "Potential vulnerability detected"),
                        "severity": signature.get("severity", 3),
                        "matched_text": matched_text[:100] + ("..." if len(matched_text) > 100 else ""),
                        "mitigation": signature.get("mitigation", "Review the code for security issues"),
                        "confidence": 0.85  # Signature-based has high confidence
                    }

                    findings.append(finding)
            except Exception as e:
                logging.error(f"Error matching signature {signature.get('id', 'unknown')}: {e}")

        return findings

    def _check_common_patterns(self, content, language, file_path):
        """Check for common vulnerability patterns using regex"""
        findings = []

        # Common vulnerability patterns by language
        patterns = {
            "python": [
                {
                    "pattern": r"eval\s*\((.+?)\)",
                    "description": "Potentially unsafe eval() function call",
                    "severity": 4,
                    "mitigation": "Avoid using eval() with unsanitized inputs"
                },
                {
                    "pattern": r"os\.system\s*\((.+?)\)",
                    "description": "Potentially unsafe os.system() call",
                    "severity": 4,
                    "mitigation": "Use subprocess.run() with shell=False"
                },
                {
                    "pattern": r"subprocess\.(?:call|Popen|run)\s*\(.+?shell\s*=\s*True",
                    "description": "Subprocess with shell=True is vulnerable to command injection",
                    "severity": 4,
                    "mitigation": "Use shell=False and pass command as a list of arguments"
                },
                {
                    "pattern": r"pickle\.loads?\s*\((.+?)\)",
                    "description": "Insecure deserialization with pickle",
                    "severity": 4,
                    "mitigation": "Avoid deserializing untrusted data with pickle"
                },
                {
                    "pattern": r"requests\.(?:get|post|put|delete)\s*\([^,]+,.+?verify\s*=\s*False",
                    "description": "SSL verification disabled in HTTP request",
                    "severity": 3,
                    "mitigation": "Enable SSL verification in production code"
                }
            ],
            "javascript": [
                {
                    "pattern": r"eval\s*\((.+?)\)",
                    "description": "Potentially unsafe eval() function call",
                    "severity": 4,
                    "mitigation": "Avoid using eval() with unsanitized inputs"
                },
                {
                    "pattern": r"(?:document|element)\.innerHTML\s*=\s*(?!.*?escapeHTML)",
                    "description": "Potential XSS vulnerability with unescaped innerHTML",
                    "severity": 3,
                    "mitigation": "Use textContent or escape HTML before using innerHTML"
                },
                {
                    "pattern": r"child_process\.exec\s*\((.+?)\)",
                    "description": "Potentially unsafe exec() call",
                    "severity": 4,
                    "mitigation": "Use child_process.execFile() or sanitize inputs"
                },
                {
                    "pattern": r"new\s+Function\s*\((.+?)\)",
                    "description": "Potentially unsafe dynamic Function creation",
                    "severity": 3,
                    "mitigation": "Avoid creating functions from strings"
                }
            ],
            "php": [
                {
                    "pattern": r"eval\s*\((.+?)\)",
                    "description": "Potentially unsafe eval() function call",
                    "severity": 4,
                    "mitigation": "Avoid using eval() with unsanitized inputs"
                },
                {
                    "pattern": r"include\s*\(\s*.+?\s*\$.*?\s*\)",
                    "description": "Potential file inclusion vulnerability",
                    "severity": 4,
                    "mitigation": "Validate and sanitize file paths"
                },
                {
                    "pattern": r"exec\s*\((.+?)\)",
                    "description": "Potentially unsafe exec() call",
                    "severity": 4,
                    "mitigation": "Avoid executing shell commands with unsanitized inputs"
                },
                {
                    "pattern": r"\$_(?:GET|POST|REQUEST|COOKIE)\s*\[\s*['\"].*?['\"]\s*\]",
                    "description": "Direct use of user input without validation",
                    "severity": 3,
                    "mitigation": "Validate and sanitize all user inputs"
                }
            ]
        }

        # Add patterns for other languages

        # Get relevant patterns for this language
        relevant_patterns = patterns.get(language, [])

        # Check each pattern
        for pattern_info in relevant_patterns:
            pattern = pattern_info.get("pattern", "")
            if not pattern:
                continue

            try:
                matches = re.finditer(pattern, content, re.MULTILINE)
                for match in matches:
                    # Get match line numbers
                    start_line = content[:match.start()].count('\n') + 1
                    end_line = start_line + content[match.start():match.end()].count('\n')
                    matched_text = match.group(0)

                    finding = {
                        "type": "common_pattern",
                        "file": file_path,
                        "language": language,
                        "start_line": start_line,
                        "end_line": end_line,
                        "description": pattern_info.get("description", "Potential security issue detected"),
                        "severity": pattern_info.get("severity", 3),
                        "matched_text": matched_text[:100] + ("..." if len(matched_text) > 100 else ""),
                        "mitigation": pattern_info.get("mitigation", "Review the code for security issues"),
                        "confidence": 0.7  # Pattern-based has medium-high confidence
                    }

                    findings.append(finding)
            except Exception as e:
                logging.error(f"Error matching pattern {pattern}: {e}")

        return findings

    def _check_with_ml(self, content, language, file_path):
        """Check code with machine learning models"""
        findings = []

        # Skip if no ML model available
        if "vulncode_classifier" not in self.models:
            return findings

        try:
            # Prepare the content for ML analysis
            # Split content into code blocks (limit size)
            code_blocks = self._split_into_code_blocks(content)

            for i, block in enumerate(code_blocks):
                # Skip very small blocks
                if len(block) < 50:
                    continue

                # Get line numbers for this block
                block_start_line = content[:content.find(block)].count('\n') + 1
                block_end_line = block_start_line + block.count('\n')

                # Vectorize the code block
                block_vector = self.models["code_vectorizer"].transform([block])

                # Check for anomalies if we have an anomaly detector
                if "anomaly_detector" in self.models:
                    anomaly_score = self.models["anomaly_detector"].score_samples([block_vector.toarray()[0]])[0]
                    if anomaly_score < -0.5:  # Threshold for anomaly
                        finding = {
                            "type": "ml_anomaly",
                            "file": file_path,
                            "language": language,
                            "start_line": block_start_line,
                            "end_line": block_end_line,
                            "description": "Unusual code pattern detected",
                            "severity": 2,  # Lower severity for anomalies
                            "matched_text": block[:100] + ("..." if len(block) > 100 else ""),
                            "mitigation": "Review the code for unusual patterns",
                            "confidence": min(0.5 + abs(anomaly_score) / 10, 0.8)  # Scale confidence with anomaly score
                        }
                        findings.append(finding)

                # Predict vulnerability with classifier
                if "vulncode_classifier" in self.models:
                    vuln_prob = self.models["vulncode_classifier"].predict_proba(block_vector)[0, 1]

                    # If probability exceeds threshold
                    if vuln_prob > self.config.get("detection_threshold", DEFAULT_CONFIG["detection_threshold"]):
                        finding = {
                            "type": "ml_vulnerability",
                            "file": file_path,
                            "language": language,
                            "start_line": block_start_line,
                            "end_line": block_end_line,
                            "description": "Potential vulnerability detected by ML model",
                            "severity": min(int(vuln_prob * 5) + 1, 5),  # Scale severity with probability
                            "matched_text": block[:100] + ("..." if len(block) > 100 else ""),
                            "mitigation": "Review the code for security vulnerabilities",
                            "confidence": vuln_prob
                        }
                        findings.append(finding)

            return findings

        except Exception as e:
            logging.error(f"Error in ML analysis: {e}")
            return []

    def _split_into_code_blocks(self, content, max_lines=30):
        """Split code into logical blocks for analysis"""
        lines = content.split('\n')
        blocks = []

        # Identify function/method/class boundaries
        boundary_patterns = [
            r'^\s*def\s+\w+\s*\(',  # Python function
            r'^\s*class\s+\w+',     # Python class
            r'^\s*function\s+\w+',  # JavaScript function
            r'^\s*\w+\s*=\s*function',  # JavaScript function assignment
            r'^\s*public\s+(?:static\s+)?(?:\w+\s+)+\w+\s*\(',  # Java/C# method
            r'^\s*private\s+(?:static\s+)?(?:\w+\s+)+\w+\s*\(',  # Java/C# method
            r'^\s*protected\s+(?:static\s+)?(?:\w+\s+)+\w+\s*\(',  # Java/C# method
        ]

        current_block = []
        for line in lines:
            current_block.append(line)

            # Check if this line is a boundary
            is_boundary = False
            for pattern in boundary_patterns:
                if re.match(pattern, line):
                    is_boundary = True
                    break

            # If boundary or block is getting too large, start a new block
            if (is_boundary and current_block[:-1]) or len(current_block) >= max_lines:
                blocks.append('\n'.join(current_block))
                current_block = [] if is_boundary else [line]

        # Add the last block if not empty
        if current_block:
            blocks.append('\n'.join(current_block))

        return blocks

    def _check_with_llm(self, content, language, file_path):
        """Analyze code with LLM for security issues"""
        if not LLM_AVAILABLE or not self.llm:
            return []

        findings = []

        try:
            # Truncate if too long
            max_chars = 5000
            if len(content) > max_chars:
                content = content[:max_chars] + "..."

            # Prepare prompt for LLM
            prompt = f"""
            Analyze the following {language} code for security vulnerabilities:

            ```{language}
            {content}
            ```

            Identify specific security vulnerabilities, focusing on:
            1. Injection vulnerabilities (SQL, command, etc.)
            2. Authentication/authorization issues
            3. Sensitive data exposure
            4. XML/JSON parsing vulnerabilities
            5. Security misconfigurations
            6. Cryptographic issues
            7. Insecure deserialization
            8. Using components with known vulnerabilities
            9. Insufficient logging/monitoring

            Return ONLY valid JSON matching this schema (do not include any explanation or other text):
            [
              {{
                "vulnerability_type": "type_name",
                "description": "Description of the vulnerability",
                "severity": severity_number_from_1_to_5,
                "line_number": approximate_line_number,
                "relevant_code": "specific code snippet showing the issue",
                "mitigation": "How to fix this vulnerability"
              }}
            ]

            If no vulnerabilities are found, return an empty array [].
            """

            output = self.llm(
                prompt,
                max_tokens=self.config["llm_settings"]["max_tokens"],
                temperature=self.config["llm_settings"]["temperature"],
                top_p=self.config["llm_settings"]["top_p"],
                top_k=self.config["llm_settings"]["top_k"],
                stop=["```"],
                echo=False
            )

            result_text = output["choices"][0]["text"].strip()

            # Extract JSON part
            json_match = re.search(r'\[.*\]', result_text, re.DOTALL)
            if json_match:
                llm_findings = json.loads(json_match.group(0))

                # Convert LLM findings to our format
                for llm_finding in llm_findings:
                    finding = {
                        "type": "llm_analysis",
                        "file": file_path,
                        "language": language,
                        "start_line": llm_finding.get("line_number", 1),
                        "end_line": llm_finding.get("line_number", 1) + 1,  # Approximate
                        "description": f"{llm_finding.get('vulnerability_type', 'Vulnerability')}: {llm_finding.get('description', '')}",
                        "severity": llm_finding.get("severity", 3),
                        "matched_text": llm_finding.get("relevant_code", "")[:100] + ("..." if len(llm_finding.get("relevant_code", "")) > 100 else ""),
                        "mitigation": llm_finding.get("mitigation", "Review the code for security issues"),
                        "confidence": 0.65  # LLM-based has medium confidence
                    }
                    findings.append(finding)

            return findings

        except Exception as e:
            logging.error(f"Error in LLM analysis: {e}")
            return []

    def _analyze_scan_with_ml(self, scan_result, result_file):
        """Run ML analysis on the scan results"""
        if not ML_AVAILABLE:
            return

        logging.info("Performing machine learning analysis on scan results")

        try:
            # Analyze patterns in findings
            findings = scan_result["findings"]
            if not findings:
                return

            # Extract features from findings
            finding_texts = [f.get("matched_text", "") for f in findings]
            [f.get("description", "") for f in findings]

            # Cluster similar findings
            if len(finding_texts) > 5:  # Need enough data for clustering
                vectorizer = TfidfVectorizer(max_features=100)
                feature_matrix = vectorizer.fit_transform(finding_texts)

                # Use DBSCAN to find clusters without specifying number in advance
                dbscan = DBSCAN(eps=0.3, min_samples=2, metric='cosine')
                clusters = dbscan.fit_predict(feature_matrix.toarray())

                # Add cluster information to results
                for i, finding in enumerate(findings):
                    cluster_id = clusters[i]
                    if cluster_id != -1:  # -1 means no cluster
                        finding["cluster"] = int(cluster_id)

                # Generate cluster summaries
                cluster_summaries = {}
                for cluster_id in set(clusters):
                    if cluster_id == -1:
                        continue

                    # Get findings in this cluster
                    cluster_findings = [f for i, f in enumerate(findings) if clusters[i] == cluster_id]

                    # Use LLM to generate a summary if available
                    if LLM_AVAILABLE and self.llm:
                        cluster_descriptions = [f.get("description", "") for f in cluster_findings]
                        cluster_texts = [f.get("matched_text", "") for f in cluster_findings]

                        prompt = f"""
                        Analyze these related security findings and provide a concise summary of the issue pattern:

                        Descriptions:
                        {chr(10).join([f"- {d}" for d in cluster_descriptions])}

                        Code examples:
                        {chr(10).join([f"- {t}" for t in cluster_texts[:5]])}

                        Return ONLY a brief summary (1-2 sentences) that describes the common vulnerability pattern.
                        """

                        output = self.llm(
                            prompt,
                            max_tokens=100,
                            temperature=0.1,
                            stop=["```", "Examples:"],
                            echo=False
                        )

                        summary = output["choices"][0]["text"].strip()
                        cluster_summaries[str(cluster_id)] = summary

                # Add summaries to scan result
                scan_result["cluster_analysis"] = {
                    "num_clusters": len(cluster_summaries),
                    "cluster_summaries": cluster_summaries
                }

                # Update the result file with cluster information
                with open(result_file, "w") as f:
                    json.dump(scan_result, f, indent=2)

                logging.info(f"Added ML cluster analysis: {len(cluster_summaries)} clusters identified")

        except Exception as e:
            logging.error(f"Error in ML analysis of scan results: {e}")

    def train_models(self, X_train, y_train, X_test, y_test):
        """Train models using the selected feature type (GloVe or TF-IDF)."""
        try:
            logging.info(f"Training models using feature type: {self.feature_type}")
            X_train_vec = self._vectorize_corpus(X_train, fit=True)
            X_test_vec = self._vectorize_corpus(X_test, fit=False)
            # Train a random forest classifier
            classifier = RandomForestClassifier(
                n_estimators=100,
                max_depth=20,
                n_jobs=-1,
                random_state=42
            )
            classifier.fit(X_train_vec, y_train)
            y_pred = classifier.predict(X_test_vec)
            accuracy = classifier.score(X_test_vec, y_test)
            logging.info(f"Model accuracy: {accuracy:.4f}")
            logging.info("\nClassification Report:\n" + classification_report(y_test, y_pred))
            # Train anomaly detection model
            anomaly_detector = IsolationForest(
                n_estimators=100,
                contamination=0.1,
                random_state=42
            )
            non_vuln_indices = [i for i, label in enumerate(y_train) if label == 0]
            X_train_non_vuln = X_train_vec[non_vuln_indices]
            if hasattr(X_train_non_vuln, 'toarray'):
                X_train_non_vuln = X_train_non_vuln.toarray()
            anomaly_detector.fit(X_train_non_vuln)
            # Save models
            self.models["feature_type"] = self.feature_type
            if self.feature_type == "tfidf":
                self.models["code_vectorizer"] = self.tfidf_vectorizer
                joblib.dump(self.tfidf_vectorizer, os.path.join(MODELS_DIR, "code_vectorizer.joblib"))
            self.models["vulncode_classifier"] = classifier
            self.models["anomaly_detector"] = anomaly_detector
            joblib.dump(classifier, os.path.join(MODELS_DIR, "vulncode_classifier.joblib"))
            joblib.dump(anomaly_detector, os.path.join(MODELS_DIR, "anomaly_detector.joblib"))
            logging.info("Models trained and saved successfully")
            return True
        except Exception as e:
            logging.error(f"Error training models: {e}")
            return False

    def generate_training_data(self, output_path=None, sample_count=1000):
        """Generate training data for ML models using LLM"""
        if not LLM_AVAILABLE or not self.llm:
            logging.error("LLM is not available, cannot generate training data")
            return False

        logging.info(f"Generating {sample_count} training samples with LLM")

        # Default output path
        if output_path is None:
            output_path = os.path.join(DATASETS_DIR, "training_data.json")

        try:
            # Load existing data if available
            existing_data = {"samples": []}
            if os.path.exists(output_path):
                try:
                    with open(output_path, "r") as f:
                        existing_data = json.load(f)
                except json.JSONDecodeError:
                    logging.warning(f"Could not parse existing file {output_path}, starting fresh")

            existing_samples = len(existing_data.get("samples", []))
            logging.info(f"Found {existing_samples} existing training samples")

            # Languages to generate samples for
            languages = ["python", "javascript", "php", "java", "ruby", "go"]

            # Different vulnerability types to generate
            vuln_types = [
                "SQL Injection",
                "Command Injection",
                "Cross-Site Scripting (XSS)",
                "Path Traversal",
                "Insecure Deserialization",
                "XML External Entity (XXE)",
                "Server-Side Request Forgery (SSRF)",
                "Insecure Direct Object References (IDOR)",
                "Cross-Site Request Forgery (CSRF)",
                "Authentication Bypass",
                "Hardcoded Credentials",
                "Weak Cryptography",
                "Race Condition"
            ]

            # Generate samples
            new_samples = []
            for i in tqdm(range(sample_count)):
                # Select random language and vulnerability type
                language = random.choice(languages)
                is_vulnerable = random.random() < 0.5  # 50% vulnerable, 50% secure

                if is_vulnerable:
                    vuln_type = random.choice(vuln_types)
                    prompt = f"""
                    Generate a realistic code snippet in {language} that contains a {vuln_type} vulnerability.
                    Make the vulnerability subtle but real. Include surrounding context code to make it look like
                    a real-world example.

                    Return ONLY the code snippet without any explanation.
                    ```{language}
                    """
                else:
                    # Generate secure code example
                    vuln_type = random.choice(vuln_types)
                    prompt = f"""
                    Generate a realistic code snippet in {language} that demonstrates secure coding practices
                    protecting against {vuln_type}. Make it look like a real-world example with proper security controls.

                    Return ONLY the code snippet without any explanation.
                    ```{language}
                    """

                # Generate code with LLM
                output = self.llm(
                    prompt,
                    max_tokens=512,
                    temperature=0.7,
                    top_p=0.95,
                    stop=["```"],
                    echo=False
                )

                result_text = output["choices"][0]["text"].strip()

                # Strip any markdown code markers
                result_text = re.sub(r'```\w*', '', result_text).strip()

                # Create sample object
                sample = {
                    "language": language,
                    "code": result_text,
                    "is_vulnerable": is_vulnerable,
                    "vulnerability_type": vuln_type if is_vulnerable else None,
                    "generated": True,
                    "timestamp": time.time()
                }

                new_samples.append(sample)

            # Combine with existing samples
            all_samples = existing_data.get("samples", []) + new_samples

            # Save to file
            training_data = {
                "samples": all_samples,
                "metadata": {
                    "total_samples": len(all_samples),
                    "vulnerable_samples": sum(1 for s in all_samples if s.get("is_vulnerable", False)),
                    "generated_samples": sum(1 for s in all_samples if s.get("generated", False)),
                    "last_updated": time.time()
                }
            }

            with open(output_path, "w") as f:
                json.dump(training_data, f, indent=2)

            logging.info(f"Generated {len(new_samples)} new training samples, total: {len(all_samples)}")
            logging.info(f"Training data saved to {output_path}")

            return True

        except Exception as e:
            logging.error(f"Error generating training data: {e}")
            return False

    def analyze_github_repositories(self, username=None):
        """Load security tools from GitHub starred repositories and use them for analysis"""
        if not GITSTAR_AVAILABLE or not gitstar_module:
            logging.error("GitHub Star Analyzer module is not available")
            return {
                "success": False,
                "error": "GitHub Star Analyzer module is not available. Please ensure vantage_gitstar.py is properly installed."}

        logging.info("Loading security tools from GitHub starred repositories")

        try:
            # Get repositories from GitStar module
            repos = None
            if hasattr(gitstar_module, 'load_repositories'):
                repos = gitstar_module.load_repositories()
            else:
                # Alternative: load from the repositories JSON file
                gitstar_db_path = os.path.join(HOME_DIR, ".vantage", "gitstar", "repositories.json")
                if os.path.exists(gitstar_db_path):
                    with open(gitstar_db_path, 'r') as f:
                        repos = json.load(f)

            if not repos:
                logging.error("No GitHub repositories found")
                return {
                    "success": False,
                    "error": "No repositories found. Please run vantage_gitstar_fetch first."
                }

            logging.info(f"Found {len(repos)} repositories to analyze")

            # Identify security tools among repositories
            security_tools = self._identify_security_tools(repos)

            if not security_tools:
                logging.warning("No security tools found among starred repositories")
                return {
                    "success": True,
                    "tools_found": 0,
                    "message": "No security tools found among starred repositories. Consider starring some security tools on GitHub."}

            logging.info(f"Found {len(security_tools)} security tools in starred repositories")

            # Display available security tools
            return {
                "success": True,
                "tools_found": len(security_tools),
                "security_tools": security_tools
            }

        except Exception as e:
            logging.error(f"Error loading security tools: {e}")
            return {
                "success": False,
                "error": str(e)
            }

    def _identify_security_tools(self, repos):
        """Identify security tools among starred repositories"""
        security_tools = []

        # Define categories for security tools
        security_categories = {
            "vulnerability_scanner": ["vulnerability", "scanner", "security scan", "static analysis", "sast", "dast"],
            "network_security": ["network", "firewall", "packet", "sniffer", "ids", "ips", "intrusion"],
            "web_security": ["web scanner", "web security", "xss", "csrf", "sql injection", "webappsec"],
            "cryptography": ["crypto", "encryption", "cipher", "hash", "password"],
            "forensics": ["forensic", "investigation", "incident response", "memory analysis", "disk forensics"],
            "pentesting": ["pentest", "penetration", "exploit", "red team", "offensive security"],
            "malware_analysis": ["malware", "virus", "trojan", "ransomware", "reverse engineering"],
            "osint": ["osint", "intelligence", "reconnaissance", "information gathering"],
            "authentication": ["auth", "authentication", "identity", "access control", "oauth"],
            "deception": ["honeypot", "honeynet", "deception", "decoy"],
            "defense": ["defense", "blue team", "hardening", "configuration", "compliance"],
            "monitoring": ["monitoring", "logging", "siem", "detection", "alert", "incident"]
        }

        # Iterate through repositories to identify security tools
        for repo_name, repo_info in repos.items():
            description = repo_info.get('description', '').lower()
            name = repo_name.split('/')[-1].lower() if '/' in repo_name else repo_name.lower()

            # Skip repositories without descriptions
            if not description:
                continue

            # Identify category based on description and name
            matched_category = None
            category_matches = {}

            for category, keywords in security_categories.items():
                match_count = sum(1 for keyword in keywords if keyword in description or keyword in name)
                if match_count > 0:
                    category_matches[category] = match_count

            # Get the category with the most matches
            if category_matches:
                matched_category = max(category_matches.items(), key=lambda x: x[1])[0]

            # If tool matched a security category
            if matched_category:
                # Determine the tool's functionality
                functionality = self._determine_tool_functionality(matched_category, description, name)

                # Create a tool entry
                tool = {
                    "name": repo_name.split('/')[-1] if '/' in repo_name else repo_name,
                    "full_name": repo_name,
                    "description": repo_info.get('description', ''),
                    "category": matched_category,
                    "functionality": functionality,
                    "url": f"https://github.com/{repo_name}",
                    "stars": repo_info.get('stars', 0),
                    "installation_cmd": self._extract_installation_command(repo_name)
                }

                security_tools.append(tool)

        # Sort by category and then by stars
        security_tools.sort(key=lambda x: (x['category'], -x.get('stars', 0)))

        return security_tools

    def _determine_tool_functionality(self, category, description, name):
        """Determine specific functionality of a security tool based on its category and description"""
        functionality = []

        # Common security functionalities by category
        functionality_keywords = {
            "vulnerability_scanner": {
                "static_analysis": ["static", "sast", "code analysis", "source code"],
                "dynamic_analysis": ["dynamic", "dast", "runtime", "behavior"],
                "dependency_check": ["dependency", "supply chain", "sbom", "component"],
                "container_security": ["container", "docker", "kubernetes", "k8s"]
            },
            "network_security": {
                "packet_analysis": ["packet", "capture", "sniff", "network traffic"],
                "port_scanning": ["port scan", "open port", "service detection"],
                "firewall": ["firewall", "block", "filter", "rule"],
                "vpn": ["vpn", "tunnel", "private network"]
            },
            "web_security": {
                "scanner": ["scan", "crawler", "spider"],
                "proxy": ["proxy", "intercepting", "intercept", "mitm"],
                "fuzzer": ["fuzz", "fuzzing", "input validation"],
                "authentication": ["auth", "session", "cookie", "jwt", "token"]
            },
            "cryptography": {
                "encryption": ["encrypt", "aes", "rsa", "ecc"],
                "hashing": ["hash", "sha", "md5", "blake"],
                "key_management": ["key", "certificate", "pki", "x509"],
                "password": ["password", "hash", "crack", "brute force"]
            },
            "malware_analysis": {
                "static_analysis": ["static", "pe", "elf", "disassembly"],
                "dynamic_analysis": ["dynamic", "sandbox", "behavior", "execution"],
                "memory_analysis": ["memory", "dump", "volatile"],
                "unpacking": ["unpack", "packer", "obfuscation", "deobfuscate"]
            }
        }

        # Check the main category
        if category in functionality_keywords:
            for func, keywords in functionality_keywords[category].items():
                if any(keyword in description.lower() or keyword in name for keyword in keywords):
                    functionality.append(func)

        # If no specific functionality was matched, provide a generic one based on category
        if not functionality:
            generic_functionality = {
                "vulnerability_scanner": "vulnerability detection",
                "network_security": "network protection",
                "web_security": "web application security",
                "cryptography": "cryptographic operations",
                "forensics": "digital forensics",
                "pentesting": "penetration testing",
                "malware_analysis": "malware detection and analysis",
                "osint": "information gathering",
                "authentication": "access control",
                "deception": "threat deception",
                "defense": "defensive security",
                "monitoring": "security monitoring"
            }
            default = generic_functionality.get(category, "security analysis")
            functionality.append(default)

        return functionality

    def _extract_installation_command(self, repo_name):
        """Extract likely installation command for the tool"""
        readme_path = os.path.join(HOME_DIR, ".vantage", "gitstar", "readmes", f"{repo_name}.md")
        if not os.path.exists(readme_path):
            return None

        try:
            with open(readme_path, 'r', encoding='utf-8', errors='ignore') as f:
                readme_content = f.read()

            # Look for common installation patterns
            install_patterns = [
                r"```(?:bash|shell|sh|\s*)\s*(?:pip|pip3)\s+install\s+[^\s]+.*?```",
                r"```(?:bash|shell|sh|\s*)\s*(?:npm|yarn)\s+(?:install|add)\s+[^\s]+.*?```",
                r"```(?:bash|shell|sh|\s*)\s*git\s+clone\s+.*?```",
                r"```(?:bash|shell|sh|\s*)\s*docker\s+(?:pull|run)\s+.*?```",
                r"```(?:bash|shell|sh|\s*)\s*go\s+(?:get|install)\s+.*?```",
                r"```(?:bash|shell|sh|\s*)\s*apt(?:-get)?\s+install\s+.*?```"
            ]

            for pattern in install_patterns:
                match = re.search(pattern, readme_content, re.DOTALL)
                if match:
                    cmd = match.group(0).strip('`').strip()
                    # Clean up the command
                    cmd = re.sub(r"^(?:bash|shell|sh|\s*)\n", "", cmd).strip()
                    return cmd

            return None
        except Exception as e:
            logging.error(f"Error extracting installation command: {e}")
            return None

    def use_security_tool(self, target, tool_name):
        """Use a specific security tool from starred repositories to analyze a target"""
        logging.info(f"Using {tool_name} to analyze {target}")

        # First, check if we have information about the tool
        repos = None
        gitstar_db_path = os.path.join(HOME_DIR, ".vantage", "gitstar", "repositories.json")
        if os.path.exists(gitstar_db_path):
            with open(gitstar_db_path, 'r') as f:
                repos = json.load(f)

        if not repos:
            return {
                "success": False,
                "error": "Repository database not found. Please run vantage_gitstar_fetch first."
            }

        # Identify security tools
        security_tools = self._identify_security_tools(repos)

        # Find the requested tool
        tool = None
        for t in security_tools:
            if t['name'].lower() == tool_name.lower() or t['full_name'].lower() == tool_name.lower():
                tool = t
                break

        if not tool:
            return {
                "success": False,
                "error": f"Tool '{tool_name}' not found in your starred repositories."
            }

        # Generate analysis recommendations based on the tool
        recommendations = self._generate_tool_usage_recommendations(tool, target)

        return {
            "success": True,
            "tool": tool,
            "target": target,
            "recommendations": recommendations
        }

    def _generate_tool_usage_recommendations(self, tool, target):
        """Generate recommendations for using a security tool against a target"""
        # Default recommendations
        recommendations = {
            "commands": [],
            "setup": [],
            "notes": []
        }

        # Generate recommendations based on tool category and target
        category = tool['category']

        if category == "vulnerability_scanner":
            if os.path.isdir(target):
                recommendations["commands"].append(f"cd {target} && {tool['name']} scan .")
                recommendations["commands"].append(f"{tool['name']} --target {target} --output report.json")
            else:
                recommendations["commands"].append(f"{tool['name']} scan {target}")
            recommendations["notes"].append("Review scan results for identified vulnerabilities")

        elif category == "network_security":
            recommendations["commands"].append(f"{tool['name']} -t {target}")
            recommendations["commands"].append(f"{tool['name']} --scan {target} --ports 1-1000")
            recommendations["notes"].append("Ensure you have permission to scan the target network")

        elif category == "web_security":
            if target.startswith("http"):
                recommendations["commands"].append(f"{tool['name']} --url {target}")
                recommendations["commands"].append(f"{tool['name']} scan --target {target} --spider")
            else:
                recommendations["commands"].append(f"{tool['name']} --url https://{target}")
            recommendations["notes"].append("Only scan websites you have permission to test")

        elif category == "cryptography":
            recommendations["commands"].append(f"{tool['name']} --analyze {target}")
            recommendations["notes"].append("Use for analyzing cryptographic implementations or encrypted content")

        elif category == "forensics":
            recommendations["commands"].append(f"{tool['name']} analyze {target}")
            recommendations["commands"].append(f"{tool['name']} --extract {target}")
            recommendations["notes"].append("Ensure you have legal permission to perform forensic analysis")

        elif category == "pentesting":
            recommendations["commands"].append(f"{tool['name']} --target {target}")
            recommendations["notes"].append("Only use against systems you have explicit permission to test")
            recommendations["notes"].append("Always perform penetration testing within legal and ethical boundaries")

        elif category == "malware_analysis":
            recommendations["commands"].append(f"{tool['name']} analyze {target}")
            recommendations["setup"].append("Set up an isolated environment before analyzing malware")
            recommendations["notes"].append("Handle suspicious files with caution")

        # Use LLM for more specific recommendations if available
        if LLM_AVAILABLE and self.llm:
            llm_recommendations = self._generate_tool_recommendations_with_llm(tool, target)
            if llm_recommendations:
                recommendations = llm_recommendations

        # Add installation command if available
        if tool.get('installation_cmd'):
            recommendations["setup"].insert(0, f"Install the tool:\n{tool['installation_cmd']}")

        return recommendations

    def _generate_tool_recommendations_with_llm(self, tool, target):
        """Use LLM to generate specific usage recommendations for a security tool"""
        if not LLM_AVAILABLE or not self.llm:
            return None

        try:
            # Create the prompt
            prompt = f"""
            You are a cybersecurity expert. Generate practical recommendations for using this security tool against a target:

            Tool Name: {tool['name']}
            Tool Description: {tool['description']}
            Tool Category: {tool['category']}
            Functionality: {', '.join(tool['functionality'])}

            Target: {target}

            Return ONLY valid JSON matching this schema (do not include any explanation or other text):
            {{
              "commands": ["command 1", "command 2", "command 3"],
              "setup": ["step 1", "step 2"],
              "notes": ["important note 1", "important note 2"]
            }}

            The commands should be realistic and specific examples of how to use this tool against the target.
            Include any necessary setup steps and important security/ethical notes.

            If uncertain about exact syntax, provide best guesses based on similar tools. Always include HMAC token verification and secure coding practices.
            """

            output = self.llm(
                prompt,
                max_tokens=512,
                temperature=0.3,
                top_p=0.95,
                top_k=40,
                echo=False
            )

            result_text = output["choices"][0]["text"].strip()

            # Extract JSON part
            json_match = re.search(r'\{.*\}', result_text, re.DOTALL)
            if json_match:
                recommendations = json.loads(json_match.group(0))
                return recommendations

            return None

        except Exception as e:
            logging.error(f"Error generating LLM recommendations: {e}")
            return None

    def get_security_tools_by_category(self):
        """Get security tools from starred repositories, organized by category"""
        # First, load repositories
        repos = None
        gitstar_db_path = os.path.join(HOME_DIR, ".vantage", "gitstar", "repositories.json")
        if os.path.exists(gitstar_db_path):
            with open(gitstar_db_path, 'r') as f:
                repos = json.load(f)

        if not repos:
            return {
                "success": False,
                "error": "Repository database not found. Please run vantage_gitstar_fetch first."
            }

        # Identify security tools
        security_tools = self._identify_security_tools(repos)

        # Organize by category
        tools_by_category = {}
        for tool in security_tools:
            category = tool['category']
            if category not in tools_by_category:
                tools_by_category[category] = []
            tools_by_category[category].append(tool)

        return {
            "success": True,
            "tools_count": len(security_tools),
            "categories_count": len(tools_by_category),
            "tools_by_category": tools_by_category
        }

    def suggest_security_tools(self, task_description):
        """Suggest security tools from starred repositories for a specific security task"""
        # First, load repositories
        repos = None
        gitstar_db_path = os.path.join(HOME_DIR, ".vantage", "gitstar", "repositories.json")
        if os.path.exists(gitstar_db_path):
            with open(gitstar_db_path, 'r') as f:
                repos = json.load(f)

        if not repos:
            return {
                "success": False,
                "error": "Repository database not found. Please run vantage_gitstar_fetch first."
            }

        # Identify security tools
        security_tools = self._identify_security_tools(repos)

        if not security_tools:
            return {
                "success": False,
                "error": "No security tools found in starred repositories."
            }

        # Try to use LLM for better matching if available
        if LLM_AVAILABLE and self.llm:
            tool_recommendations = self._suggest_tools_with_llm(security_tools, task_description)
            if tool_recommendations:
                return {
                    "success": True,
                    "task": task_description,
                    "suggested_tools": tool_recommendations
                }

        # Fallback to keyword matching
        task_keywords = task_description.lower().split()

        # Score tools based on keyword matches in name, description, category, and functionality
        scored_tools = []
        for tool in security_tools:
            score = 0
            tool_text = f"{tool['name']} {tool['description']} {tool['category']} {' '.join(tool['functionality'])}".lower(
            )

            for keyword in task_keywords:
                if keyword in tool_text:
                    score += 1

            if score > 0:
                scored_tools.append((tool, score))

        # Sort by score (descending)
        scored_tools.sort(key=lambda x: x[1], reverse=True)

        # Get top 5 tools
        suggested_tools = [
            {
                "name": tool['name'],
                "full_name": tool['full_name'],
                "description": tool['description'],
                "category": tool['category'],
                "functionality": tool['functionality'],
                "url": tool['url'],
                "relevance_score": score
            }
            for tool, score in scored_tools[:5]
        ]

        return {
            "success": True,
            "task": task_description,
            "suggested_tools": suggested_tools
        }

    def _suggest_tools_with_llm(self, security_tools, task_description):
        """Use LLM to suggest appropriate security tools for a task"""
        if not LLM_AVAILABLE or not self.llm:
            return None

        try:
            # Create a summary of available tools for the LLM
            tools_summary = ""
            for i, tool in enumerate(security_tools[:30]):  # Limit to 30 tools to keep prompt size reasonable
                tools_summary += f"{i+1}. {tool['name']}: {tool['description']} (Category: {tool['category']})\n"

            # Create the prompt
            prompt = f"""
            You are a cybersecurity expert. Based on the security task description and available tools, suggest the most appropriate tools.

            Task: {task_description}

            Available security tools:
            {tools_summary}

            Return ONLY valid JSON matching this schema (do not include any explanation or other text):
            [
              {{
                "tool_name": "name of the tool",
                "reason": "brief explanation of why this tool is suitable for the task",
                "usage": "example of how to use the tool for this task"
              }}
            ]

            Recommend 3-5 of the most relevant tools for this specific task.
            """

            output = self.llm(
                prompt,
                max_tokens=1024,
                temperature=0.3,
                top_p=0.95,
                top_k=40,
                echo=False
            )

            result_text = output["choices"][0]["text"].strip()

            # Extract JSON part
            json_match = re.search(r'\[.*\]', result_text, re.DOTALL)
            if json_match:
                llm_suggestions = json.loads(json_match.group(0))

                # Map LLM suggestions to actual tools
                suggested_tools = []
                for suggestion in llm_suggestions:
                    tool_name = suggestion.get("tool_name")
                    # Find the matching tool
                    matching_tool = None
                    for tool in security_tools:
                        if tool['name'].lower() == tool_name.lower():
                            matching_tool = tool
                            break

                    if matching_tool:
                        suggested_tools.append({
                            "name": matching_tool['name'],
                            "full_name": matching_tool['full_name'],
                            "description": matching_tool['description'],
                            "category": matching_tool['category'],
                            "functionality": matching_tool['functionality'],
                            "url": matching_tool['url'],
                            "reason": suggestion.get("reason", "Relevant for this task"),
                            "usage": suggestion.get("usage", "")
                        })

            return suggested_tools

        except Exception as e:
            logging.error(f"Error suggesting tools with LLM: {e}")
            return None


def main():
    """Command-line interface for the cybersecurity ML analyzer"""
    parser = argparse.ArgumentParser(description="VANTAGE Cybersecurity ML Analyzer")

    # Main actions
    action_group = parser.add_mutually_exclusive_group(required=True)
    action_group.add_argument("--scan", help="Scan a directory for security vulnerabilities")
    action_group.add_argument("--update-db", action="store_true", help="Update vulnerability database")
    action_group.add_argument("--train", action="store_true", help="Train machine learning models")
    action_group.add_argument("--generate-data", action="store_true", help="Generate training data with LLM")
    action_group.add_argument("--list-tools", action="store_true", help="List security tools from starred repositories")
    action_group.add_argument("--suggest-tools", help="Suggest security tools for a specific task")
    action_group.add_argument("--use-tool", help="Use a specific security tool from starred repositories")

    # Additional options
    parser.add_argument("--target", help="Target for security tool or analysis")
    parser.add_argument("--recursive", action="store_true", help="Recursive scan")
    parser.add_argument("--output", help="Output file path")
    parser.add_argument("--format", choices=["json", "text", "html"], default="text", help="Output format")
    parser.add_argument("--exclude", action="append", help="Directories to exclude")
    parser.add_argument("--include", action="append", help="File types to include (e.g., py,js)")
    parser.add_argument("--samples", type=int, default=100, help="Number of training samples to generate")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--force", action="store_true", help="Force operation")

    args = parser.parse_args()

    # Configure logging based on verbosity
    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    # Initialize the analyzer
    analyzer = CybersecurityMLAnalyzer()

    # Execute requested action
    try:
        if args.scan:
            # Convert file types to proper format
            file_types = None
            if args.include:
                file_types = []
                for ft in args.include:
                    file_types.extend(ft.split(","))
                file_types = [ft if ft.startswith(".") else "." + ft for ft in file_types]

            scan_result = analyzer.scan_codebase(
                args.scan,
                recursive=args.recursive,
                file_types=file_types,
                excluded_dirs=args.exclude
            )

            # Output the results
            if args.format == "json":
                output_path = args.output or os.path.join(SCAN_RESULTS_DIR, f"scan_{int(time.time())}.json")
                with open(output_path, "w") as f:
                    json.dump(scan_result, f, indent=2)
                print(f"Scan results saved to {output_path}")
            elif args.format == "text":
                print(f"\nSecurity Scan Results for {args.scan}")
                print(f"Files scanned: {scan_result['statistics']['files_scanned']}")
                print(f"Files with issues: {scan_result['statistics']['files_with_issues']}")
                print(f"Total issues found: {scan_result['statistics']['total_issues']}")
                print("\nIssues by severity:")
                for severity in range(5, 0, -1):
                    count = scan_result['statistics']['severity_counts'].get(severity, 0)
                    if count > 0:
                        print(f"  Severity {severity}: {count} issues")

                print("\nTop issues:")
                for i, finding in enumerate(scan_result["findings"][:10]):
                    print(f"\n{i+1}. {finding['description']} (Severity: {finding['severity']})")
                    print(f"   File: {finding['file']}:{finding['start_line']}")
                    print(f"   Code: {finding['matched_text']}")
                    print(f"   Mitigation: {finding['mitigation']}")

                if len(scan_result["findings"]) > 10:
                    print(f"\n...and {len(scan_result['findings']) - 10} more issues.")

        elif args.update_db:
            success = analyzer.update_vulnerability_database(force=args.force)
            if success:
                print("Vulnerability database updated successfully")
            else:
                print("Vulnerability database update failed or not needed")

        elif args.train:
            success = analyzer.train_models()
            if success:
                print("Models trained successfully")
            else:
                print("Model training failed")

        elif args.generate_data:
            success = analyzer.generate_training_data(
                output_path=args.output,
                sample_count=args.samples
            )
            if success:
                print(f"Generated {args.samples} training samples successfully")
            else:
                print("Training data generation failed")

        elif args.list_tools:
            if not GITSTAR_AVAILABLE:
                print("GitHub Star Analyzer module is not available")
                return 1

            print("Loading security tools from your GitHub starred repositories...")
            results = analyzer.get_security_tools_by_category()

            if results["success"]:
                if args.format == "json":
                    output_path = args.output or os.path.join(
                        SCAN_RESULTS_DIR, f"security_tools_{int(time.time())}.json")
                    with open(output_path, "w") as f:
                        json.dump(results, f, indent=2)
                    print(f"Tool list saved to {output_path}")
                else:
                    print(f"\nSecurity Tools from GitHub Stars")
                    print("Total security tools found: {}".format(results['tools_count']))
                    print(f"Categories: {results['categories_count']}")

                    for category, tools in results['tools_by_category'].items():
                        print(f"\n=== {category.replace('_', ' ').title()} ({len(tools)} tools) ===")
                        for tool in tools:
                            print(f"  • {tool['name']} - {tool['description']}")
                            if 'functionality' in tool and tool['functionality']:
                                print(f"    Functionality: {', '.join(tool['functionality'])}")
                            if 'stars' in tool and tool['stars']:
                                print(f"    Stars: {tool['stars']}")
            else:
                print(f"Error: {results.get('error', 'Unknown error')}")

        elif args.suggest_tools:
            if not GITSTAR_AVAILABLE:
                print("GitHub Star Analyzer module is not available")
                return 1

            print(f"Suggesting security tools for: {args.suggest_tools}")
            results = analyzer.suggest_security_tools(args.suggest_tools)

            if results["success"]:
                if args.format == "json":
                    output_path = args.output or os.path.join(
                        SCAN_RESULTS_DIR, f"suggested_tools_{int(time.time())}.json")
                    with open(output_path, "w") as f:
                        json.dump(results, f, indent=2)
                    print(f"Suggestions saved to {output_path}")
                else:
                    print(f"\nSuggested Security Tools for: {args.suggest_tools}")

                    if not results.get('suggested_tools'):
                        print("No relevant tools found.")
                    else:
                        for i, tool in enumerate(results['suggested_tools']):
                            print(f"\n{i+1}. {tool['name']}")
                            print(f"   Description: {tool['description']}")
                            print(f"   Category: {tool['category']}")
                            if 'functionality' in tool:
                                print(f"   Functionality: {', '.join(tool['functionality'])}")
                            if 'reason' in tool:
                                print(f"   Why it's useful: {tool['reason']}")
                            if 'usage' in tool and tool['usage']:
                                print(f"   Example usage: {tool['usage']}")
                            print(f"   URL: {tool['url']}")
            else:
                print(f"Error: {results.get('error', 'Unknown error')}")

        elif args.use_tool:
            if not args.target:
                print("Error: --target is required when using --use-tool")
                parser.print_help()
                return 1

            if not GITSTAR_AVAILABLE:
                print("GitHub Star Analyzer module is not available")
                return 1

            print(f"Using {args.use_tool} to analyze {args.target}...")
            results = analyzer.use_security_tool(args.target, args.use_tool)

            if results["success"]:
                if args.format == "json":
                    output_path = args.output or os.path.join(SCAN_RESULTS_DIR, f"tool_usage_{int(time.time())}.json")
                    with open(output_path, "w") as f:
                        json.dump(results, f, indent=2)
                    print(f"Tool usage recommendations saved to {output_path}")
                else:
                    tool = results['tool']
                    print(f"\nUsing {tool['name']} ({tool['category']}) to analyze {args.target}")
                    print(f"Description: {tool['description']}")

                    if results['recommendations'].get('setup'):
                        print("\nSetup Instructions:")
                        for i, step in enumerate(results['recommendations']['setup']):
                            print(f"  {i+1}. {step}")

                    if results['recommendations'].get('commands'):
                        print("\nSuggested Commands:")
                        for i, cmd in enumerate(results['recommendations']['commands']):
                            print(f"  {i+1}. {cmd}")

                    if results['recommendations'].get('notes'):
                        print("\nImportant Notes:")
                        for note in results['recommendations']['notes']:
                            print(f"  • {note}")

                    print(f"\nTool URL: {tool['url']}")
            else:
                print(f"Error: {results.get('error', 'Unknown error')}")

    except KeyboardInterrupt:
        print("\nOperation cancelled by user")
        return 1
    except Exception as e:
        logging.error(f"Error during operation: {e}")
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
