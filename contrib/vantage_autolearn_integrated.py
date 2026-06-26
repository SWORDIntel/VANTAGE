#!/usr/bin/env python3
"""
VANTAGE Autolearn with Python-Bash Integration
Enhanced version using unified state management and IPC
"""

import os
import sys
import json
import time
from pathlib import Path
import atexit

# Add integration library to path
sys.path.insert(0, os.path.expanduser('~/.config/vantage/lib'))

try:
    from vantage_integration import vantage
except ImportError:
    print("ERROR: vantage_integration module not found.")
    print("Please ensure python_integration.module is loaded.")
    sys.exit(1)

# Third-party imports
try:
    import markovify
    has_markovify = True
except ImportError:
    has_markovify = False
    vantage.logger.warning("markovify not installed. Limited functionality available.")

# Check for OpenVINO
has_openvino = False
try:
    import openvino as ov
    has_openvino = True
    vantage.logger.info("OpenVINO detected and enabled for accelerated suggestions")
except ImportError:
    pass

class VantageAutolearn:
    """Enhanced autolearn system with bash integration"""
    
    def __init__(self):
        # Use integrated paths
        self.state_dir = Path(vantage.state_dir) / 'ml'
        self.state_dir.mkdir(parents=True, exist_ok=True)
        
        # File paths
        self.history_file = self.state_dir / 'command_history.json'
        self.model_file = self.state_dir / 'command_model.json'
        self.stats_file = self.state_dir / 'command_stats.json'
        
        # Load existing data
        self.command_stats = self._load_stats()
        self.model = self._load_model()
        
        # Register cleanup
        atexit.register(self.save_state)
        
    def _load_stats(self):
        """Load command statistics from integrated storage"""
        stats = vantage.get_state('ml_command_stats')
        if stats:
            try:
                return json.loads(stats)
            except:
                pass
        
        # Fallback to file
        if self.stats_file.exists():
            try:
                return json.load(open(self.stats_file))
            except:
                pass
        
        return {}
    
    def _load_model(self):
        """Load Markov model"""
        if not has_markovify:
            return None
            
        if self.model_file.exists():
            try:
                with open(self.model_file, 'r') as f:
                    model_json = json.load(f)
                return markovify.Text.from_json(model_json)
            except Exception as e:
                vantage.logger.error(f"Failed to load model: {e}")
        
        return None
    
    def save_state(self):
        """Save state using integrated storage"""
        # Save stats to both state and file
        stats_json = json.dumps(self.command_stats)
        vantage.set_state('ml_command_stats', stats_json)
        
        with open(self.stats_file, 'w') as f:
            json.dump(self.command_stats, f, indent=2)
        
        vantage.logger.info("Autolearn state saved")
    
    def update_from_bash_history(self):
        """Update model from synchronized bash history"""
        if not self.history_file.exists():
            vantage.logger.warning("No command history found. Run ml-sync first.")
            return
        
        try:
            with open(self.history_file, 'r') as f:
                history_data = json.load(f)
            
            # Extract commands
            commands = [item['command'] for item in history_data]
            
            # Update statistics
            for cmd in commands:
                base_cmd = cmd.split()[0] if cmd else ''
                self.command_stats[base_cmd] = self.command_stats.get(base_cmd, 0) + 1
            
            # Update Markov model
            if has_markovify and commands:
                text = '\n'.join(commands)
                new_model = markovify.Text(text, state_size=2)
                
                if self.model:
                    # Combine with existing model
                    self.model = markovify.combine([self.model, new_model], [1, 1])
                else:
                    self.model = new_model
                
                # Save model
                with open(self.model_file, 'w') as f:
                    json.dump(self.model.to_json(), f)
                
                vantage.logger.info(f"Model updated with {len(commands)} commands")
                
        except Exception as e:
            vantage.logger.error(f"Failed to update from history: {e}")
    
    def get_suggestions(self, partial_command, n=5):
        """Get command suggestions using ML"""
        suggestions = []
        
        # First, check exact matches from stats
        for cmd, count in sorted(self.command_stats.items(), 
                                key=lambda x: x[1], reverse=True):
            if cmd.startswith(partial_command):
                suggestions.append((cmd, count))
                if len(suggestions) >= n:
                    break
        
        # If we have markovify, generate some suggestions
        if has_markovify and self.model and len(suggestions) < n:
            try:
                for _ in range(10):  # Try to generate some suggestions
                    sentence = self.model.make_sentence(tries=10)
                    if sentence and sentence.startswith(partial_command):
                        suggestions.append((sentence, 0))
                        if len(suggestions) >= n:
                            break
            except:
                pass
        
        return suggestions[:n]
    
    def send_suggestion_to_bash(self, suggestion):
        """Send suggestion back to bash via IPC"""
        try:
            # Create IPC channel if not exists
            result = vantage.bash_exec('vantage_ipc_create_channel ml_suggestions')
            
            # Send suggestion
            vantage.ipc_send('ml_suggestions', suggestion)
            vantage.logger.info(f"Sent suggestion to bash: {suggestion}")
            
        except Exception as e:
            vantage.logger.error(f"Failed to send suggestion: {e}")

def main():
    """Main entry point"""
    autolearn = VantageAutolearn()
    
    # Check command line arguments
    if len(sys.argv) > 1:
        command = sys.argv[1]
        
        if command == 'update':
            print("Updating model from bash history...")
            autolearn.update_from_bash_history()
            autolearn.save_state()
            print("Model updated successfully")
            
        elif command == 'suggest':
            if len(sys.argv) > 2:
                partial = ' '.join(sys.argv[2:])
                suggestions = autolearn.get_suggestions(partial)
                
                if suggestions:
                    print(f"Suggestions for '{partial}':")
                    for i, (cmd, count) in enumerate(suggestions, 1):
                        if count > 0:
                            print(f"  {i}. {cmd} (used {count} times)")
                        else:
                            print(f"  {i}. {cmd} (generated)")
                    
                    # Send top suggestion via IPC
                    if suggestions:
                        autolearn.send_suggestion_to_bash(suggestions[0][0])
                else:
                    print(f"No suggestions found for '{partial}'")
            else:
                print("Usage: vantage_autolearn_integrated.py suggest <partial_command>")
                
        elif command == 'stats':
            print("Command Statistics:")
            for cmd, count in sorted(autolearn.command_stats.items(), 
                                    key=lambda x: x[1], reverse=True)[:20]:
                print(f"  {cmd}: {count}")
                
        else:
            print(f"Unknown command: {command}")
            print("Available commands: update, suggest, stats")
    else:
        print("VANTAGE Autolearn Integrated")
        print("Usage: vantage_autolearn_integrated.py <command> [args]")
        print("Commands:")
        print("  update    - Update model from bash history")
        print("  suggest   - Get suggestions for partial command")
        print("  stats     - Show command statistics")

if __name__ == '__main__':
    main()