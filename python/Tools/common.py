from dataclasses import dataclass
from typing import Any

from pandas import DataFrame


def CalculateReduction(df: DataFrame, base_model: str, col: str):
    base_row = df[df["Model"] == base_model]

    if base_row.empty:
        df[col] = float("nan")
        return df

    c_base = base_row[col].item()
    df[f"{col} Reduction"] = (c_base - df[col]) / (c_base if c_base != 0 else 1) * 100

    return df


def extract_desc(filename: str) -> tuple[str, list[str | dict[str, Any]]]:
    prefix = filename[: filename.rfind("[")]
    desc = filename[filename.rfind("[") + 1 : filename.rfind("]")]
    desc = desc.split(",")
    dict_data = {x[: x.find("=")]: x[x.find("=") + 1 :] for x in desc if "=" in x}
    desc = [x for x in desc if "=" not in x]
    desc += [dict_data]
    return (prefix, desc)


def sort_key(filename):
    desc = extract_desc(filename)[1]
    if "model" in desc[-1] and isinstance(desc[-1], dict):
        return (filename, desc[0], desc[-1]["model"])
    return (filename, desc[0])


@dataclass
class OutputLog:
    trace_path: str
    cache_size: int
    ignore_obj_size: bool
    miss_ratio: float
    n_req: int
    n_promoted: int
    n_hit: int
    n_miss: int


def ordinal(n):
    n = int(n)
    if 10 <= n % 100 <= 20:
        suffix = "th"
    else:
        suffix = {1: "st", 2: "nd", 3: "rd"}.get(n % 10, "th")
    return f"{n}{suffix}"
