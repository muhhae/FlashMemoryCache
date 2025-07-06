import json
import os
import re
from pathlib import Path
from pprint import pprint
from typing import List, cast

import pandas as pd
from common import extract_desc


def ProcessResultJSON(result: dict, file):
    prefix, desc = extract_desc(file)
    metrics = result["metrics"]
    dram = None
    flash = None

    if len(metrics) > 1:
        dram = metrics[0]
        flash = metrics[1]
    else:
        flash = metrics[0]

    result = {
        "Flash Admission Treshold": flash["admission_treshold"],
        "Algorithm": flash["algorithm"],
        "Inserted": flash["inserted"],
        "Reinserted": flash["reinserted"],
        "Write": flash["reinserted"] + flash["inserted"],
        "Flash Miss Ratio": flash["miss_ratio"],
        "Overall Miss Ratio": result["miss_ratio"],
        "Flash Hit": flash["hit"],
        "Overall Hit": result["hit"],
        "Flash Request": flash["req"],
        "Overall Request": result["req"],
        "Trace": os.path.basename(prefix),
        "JSON File": os.path.basename(file),
        "Cache Size": float(cast(str, desc[0])),
        "DRAM Algorithm": dram["algorithm"] if dram is not None else "none",
        "DRAM Miss Ratio": dram["miss_ratio"] if dram is not None else 0,
        "DRAM Hit": dram["hit"] if dram is not None else 0,
        "DRAM Request": dram["req"] if dram is not None else 0,
        "DRAM Size": float(desc[-1]["dram_size"])
        if isinstance(desc[-1], dict) and "dram_size" in desc[-1]
        else 0.01
        if dram is not None
        else 0,
        "Ignore Obj Size": desc.count("ignore_obj_size"),
    }
    return result


def GetOfflineClockResult(paths: List[str]):
    tmp = []
    names = ["CLOCK", "Offline CLOCK"]
    for file in paths:
        if Path(file).stat().st_size == 0:
            continue
        f = open(file, "r")
        j = json.load(f)
        f.close()
        for i, result in enumerate(j["results"]):
            if i > 1:
                break
            j = ProcessResultJSON(result, file)
            if j["Algorithm"] != "offline-clock":
                continue
            j["Algorithm"] = names[i]
            tmp.append(j)
    return pd.DataFrame(tmp)


def GetOtherResult(paths: List[str], plot_name: str, json_name: str):
    tmp = []
    for file in paths:
        if Path(file).stat().st_size == 0:
            continue
        f = open(file, "r")
        j = json.load(f)
        f.close()
        r = ProcessResultJSON(j["results"][0], file)
        if r["Algorithm"] != json_name:
            continue
        r["Algorithm"] = plot_name
        tmp.append(r)
    return pd.DataFrame(tmp)
