import unittest
import os
import sys

# Add the installer directory to the path so we can import config
sys.path.append(os.path.abspath(os.path.join(os.path.dirname(__file__), '..', 'installer')))

from config import parse_yaml, export_variables

class TestConfig(unittest.TestCase):

    def setUp(self):
        self.test_yaml_content = {
            'key1': 'value1',
            'key2': {
                'nested_key1': 'nested_value1',
                'nested_key2': 123
            }
        }
        self.test_yaml_file = 'test.yaml'
        with open(self.test_yaml_file, 'w') as f:
            import yaml
            yaml.dump(self.test_yaml_content, f)

    def tearDown(self):
        os.remove(self.test_yaml_file)

    def test_parse_yaml(self):
        config = parse_yaml(self.test_yaml_file)
        self.assertEqual(config, self.test_yaml_content)

    def test_export_variables(self):
        import io
        from contextlib import redirect_stdout

        config = self.test_yaml_content
        f = io.StringIO()
        with redirect_stdout(f):
            export_variables(config)
        output = f.getvalue()

        expected_output = "export KEY1='value1'\nexport KEY2_NESTED_KEY1='nested_value1'\nexport KEY2_NESTED_KEY2='123'\n"
        self.assertEqual(output.strip(), expected_output.strip())

if __name__ == '__main__':
    unittest.main()
