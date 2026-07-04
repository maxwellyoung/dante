#!/usr/bin/env python3
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from model_backfill_common import backfill_asset

backfill_asset("floor_lamp", material="MAT_BRASS", color="WHITE")
