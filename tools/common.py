import argparse
import sys
import os

# Copied from https://stackoverflow.com/a/39855753
class add_path:
    def __init__(self, path):
        self.path = path

    def __enter__(self):
        sys.path.insert(0, self.path)

    def __exit__(self, exc_type, exc_value, traceback):
        try:
            sys.path.remove(self.path)
        except ValueError:
            pass


def idf_monitor_modules():
    """
    Context in which idf monitor modules are importable (i.e. are in sys.path)

    """
    return add_path(os.path.join(os.environ["IDF_PATH"], "tools"))


def parse_extension_args():
    parser = argparse.ArgumentParser()

    parser.add_argument(
        "--hide-erpc",
        help="Hide ERPC payload logs",
        action="store_true",
    )
    args, left = parser.parse_known_args()
    sys.argv = sys.argv[:1] + left
    return args
