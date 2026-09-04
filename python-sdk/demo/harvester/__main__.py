import sys
from pathlib import Path

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
    from demo.harvester.main import run
else:
    from .main import run

if __name__ == "__main__":
    run()
