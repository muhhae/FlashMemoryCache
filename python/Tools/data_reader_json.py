import json
from logging import disable
import os
from pathlib import Path
from pprint import pprint
from typing import Any, List, cast

import pandas as pd
from common import extract_desc


def ProcessResultJSON(result: dict, file, algorithm):
    prefix, desc = extract_desc(file)
    add_desc: dict[str, Any] = desc[-1] if isinstance(desc[-1], dict) else dict()
    metrics = result["metrics"]
    dram = None
    flash = None

    if len(metrics) > 1:
        dram = metrics[0]
        flash = metrics[1]
    else:
        flash = metrics[0]
    metrics_time = None
    if "metrics_time" in flash:
        metrics_time = pd.DataFrame(flash["metrics_time"][1:])
        metrics_time["byte_write"] = (
            metrics_time["byte_inserted"] + metrics_time["byte_reinserted"]
        )
        metrics_time["Algorithm"] = algorithm
        metrics_time["DRAM Size"] = (
            float(add_desc["dram_size"])
            if "dram_size" in add_desc
            else 0.01
            if dram is not None
            else 0
        )
        metrics_time["Flash Admission Treshold"] = flash["admission_treshold"]
    result = {
        "Admissioner": add_desc.get("admissioner", "none"),
        "Admission Threshold Method": add_desc.get("threshold", "lifetime"),
        "Flash Admission Treshold": flash.get("admission_treshold", 0),
        "Algorithm": algorithm,
        "Inserted": flash.get("inserted", 0),
        "Reinserted": flash.get("reinserted", 0),
        "Write": flash.get("reinserted", 0) + flash.get("inserted", 0),
        "Byte Inserted": flash.get("byte_inserted", 0),
        "Byte Reinserted": flash.get("byte_reinserted", 0),
        "Byte Write": flash.get("byte_reinserted", 0) + flash.get("byte_inserted", 0),
        "Byte Flash Read": flash.get("byte_read", 0),
        "Byte Flash Miss": flash.get("byte_miss", 0),
        "Flash Miss Ratio": flash.get("miss_ratio", 0),
        "Overall Miss Ratio": result.get("miss_ratio", 0),
        "Flash Hit": flash.get("hit", 0),
        "Overall Hit": result.get("hit", 0),
        "Flash Request": flash.get("req", 0),
        "Overall Request": result.get("req", 0),
        "Trace": os.path.basename(prefix),
        "JSON File": os.path.basename(file),
        "Cache Size": float(cast(str, desc[0])),
        "DRAM Algorithm": dram["algorithm"] if dram is not None else "none",
        "DRAM Miss Ratio": dram["miss_ratio"] if dram is not None else 0,
        "DRAM Hit": dram["hit"] if dram is not None else 0,
        "DRAM Request": dram["req"] if dram is not None else 0,
        "DRAM Size": float(add_desc["dram_size"])
        if "dram_size" in add_desc
        else 0.01
        if dram is not None
        else 0,
        "Ignore Obj Size": desc.count("ignore_obj_size"),
        "Flash Metrics Time": metrics_time,
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
            j = ProcessResultJSON(result, file, names[i])
            tmp.append(j)
    return pd.DataFrame(tmp)


def GetOtherResult(paths: List[str], plot_name: str):
    tmp = []
    for file in paths:
        if Path(file).stat().st_size == 0:
            continue
        f = open(file, "r")
        j = json.load(f)
        f.close()
        r = ProcessResultJSON(j["results"][0], file, plot_name)
        tmp.append(r)
    return pd.DataFrame(tmp)
