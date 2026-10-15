"""Run the copied Blender Lab provider without rebuilding its Python package."""

import os
from pathlib import Path
import sys

sys.path.insert(0, str(Path(os.environ["BADDIE_BLENDER_TOOLS"]) / "provider" / "mcp"))

from blmcp import main

raise SystemExit(main())
