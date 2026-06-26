#!/usr/bin/env python
# PYTHON_ARGCOMPLETE_OK
import argparse
import argcomplete # type: ignore

def main():
    parser = argparse.ArgumentParser(description="VANTAGE File Processor")
    parser.add_argument(
        "file_to_process",
        help="The file to be processed."
    )
    parser.add_argument(
        "--mode",
        choices=["fast", "thorough", "experimental"],
        default="thorough",
        help="Processing mode."
    )
    parser.add_argument(
        "--retries",
        type=int,
        default=3,
        help="Number of retries on failure."
    )
    parser.add_argument(
        "--no-backup",
        action="store_true",
        help="If set, no backup of the file will be created."
    )

    argcomplete.autocomplete(parser)

    try:
        args = parser.parse_args()

        print(f"Processing file: {args.file_to_process}")
        print(f"Mode: {args.mode}")
        print(f"Retries: {args.retries}")
        if args.no_backup:
            print("Backup: Disabled")
        else:
            print("Backup: Enabled")
        # Actual processing logic would go here
    except SystemExit as e:
        # argcomplete can cause a SystemExit when completions are printed
        # We only want to exit with an error code if it's a real arg parsing error
        if e.code != 0 and not hasattr(args, 'comp_TYPE'): # Check if it's an argcomplete exit
             raise
    except Exception as e:
        # In a real app, handle specific exceptions
        print(f"An error occurred: {e}")
        # Potentially exit with a non-zero code depending on the error
        # For this dummy script, we'll just print and exit cleanly for completion purposes.
        pass


if __name__ == "__main__":
    main()
