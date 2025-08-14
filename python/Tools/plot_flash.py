import multiprocessing
import os
import pickle
from pprint import pprint
from glob import glob
from pathlib import Path
import sys
from typing import Final

import pandas as pd
from pandas.core.apply import com
from common import extract_desc, sort_key
from data_reader_json import GetOfflineClockResult, GetOtherResult
from docs_writer import DocsWriter
from pandas.core.frame import itertools
from plotly.graph_objs import Figure
from plotly_wrapper import Line, Scatter, VerticalCompositionBar
from tabulate import tabulate

OUTPUT_PATH: Final[str] = "../../../FlashMemoryCacheDocs/figures/"
DATA_PATH: Final[str] = "../../../FlashMemoryCacheResults/"


def CreateFlashWriteComposition(df: pd.DataFrame) -> Figure:
    return VerticalCompositionBar(
        df,
        X="Model",
        Ys=[
            ("Miss", "Cache Miss"),
            ("Promotion", "Reinsertion"),
        ],
        title="Flash Write (Reinsertion + Miss) by Algorithm",
        yaxis_title="Flash Write",
        xaxis_title="Algorithm",
        mode="stack",
    )


def WriteIndividualV2(
    writer: DocsWriter,
    df: pd.DataFrame,
    add_desc: str,
    category: str,
    numeric_modifier_continuous: str,
):
    writer.Write(f"# Individual Result {add_desc} \n")
    category_order = sorted(df[category].unique())
    for s in sorted(df["Trace"].unique()):
        writer.Write(f"## {s}  \n")
        for t in sorted(df["Cache Size"].unique()):
            writer.Write(f"### {t * 100}%  \n")
            data = df.query("`Cache Size` == @t and `Trace` == @s").sort_values(
                by=numeric_modifier_continuous,
            )
            for y in [
                "Overall Miss Ratio",
                "Flash Miss Ratio",
                "Write",
                "Reinserted",
                "Inserted",
                "Byte Write",
                "Byte Reinserted",
                "Byte Inserted",
                "Byte Flash Miss",
                "Byte Flash Read",
            ]:
                writer.Write(
                    f"#### Effects of {numeric_modifier_continuous} on {y}  \n",
                )
                writer.WriteFig(
                    Line(
                        data,
                        x=numeric_modifier_continuous,
                        y=y,
                        color=category,
                        category_orders={category: category_order},
                    ),
                )
                writer.WriteFig(
                    Line(
                        data,
                        x=numeric_modifier_continuous,
                        y=y,
                        size="Byte" in y,
                        color=category,
                        category_orders={category: category_order},
                        include_zero=True,
                    ),
                )
            timeline_list: list[pd.DataFrame] = []
            timeline_list = [
                x
                for x in data["Flash Metrics Time"].tolist()
                if x is not None and isinstance(x, pd.DataFrame)
            ]
            if len(timeline_list):
                timeline: pd.DataFrame = pd.concat(timeline_list)
                timeline = timeline.sort_index()
                for t in [
                    "byte_write",
                    "byte_inserted",
                    "byte_reinserted",
                    "byte_read",
                    "byte_miss",
                    "req",
                    "hit",
                    "miss_ratio",
                    "inserted",
                    "reinserted",
                ]:
                    writer.Write(f"#### {t} Timeline  \n")
                    for n in sorted(timeline[numeric_modifier_continuous].unique()):
                        writer.Write(f"##### {numeric_modifier_continuous}: {n}  \n")
                        writer.WriteFig(
                            Line(
                                timeline.query(
                                    f"`{numeric_modifier_continuous}` == @n"
                                ),
                                x=None,
                                y=t,
                                size="byte" in t,
                                color=category,
                                category_orders={category: category_order},
                                labels={
                                    "index": "Hour",
                                    t: f"{t.replace('_', ' ').title()} / Hour",
                                },
                            ),
                        )
            for n in sorted(data[numeric_modifier_continuous].unique()):
                writer.Write(f"#### {numeric_modifier_continuous}: {n}  \n")
                writer.Write(
                    "##### Overall Miss Ratio and Write  \n",
                )
                tmp = data.query(f"`{numeric_modifier_continuous}` == @n")
                writer.WriteFig(
                    Scatter(
                        tmp,
                        x="Write",
                        y="Overall Miss Ratio",
                        color=category,
                        category_orders={category: category_order},
                    ),
                )
                writer.WriteFig(
                    Scatter(
                        tmp,
                        x="Write",
                        y="Overall Miss Ratio",
                        color=category,
                        category_orders={category: category_order},
                        include_zero=True,
                    ),
                )
                writer.Write("#### Inserted + Reinserted  \n")
                writer.WriteFig(
                    VerticalCompositionBar(
                        tmp,
                        X=category,
                        Ys=[
                            "Inserted",
                            "Reinserted",
                        ],
                        title=f"Flash Write (Inserted + Reinserted) by {category}",
                        yaxis_title="Flash Write",
                        xaxis_title=category,
                        mode="stack",
                    ),
                )
                writer.Write("##### Flash Hit and DRAM Hit  \n")
                writer.WriteFig(
                    VerticalCompositionBar(
                        data.query(f"`{numeric_modifier_continuous}` == @n"),
                        X=category,
                        Ys=[
                            "Flash Hit",
                            "DRAM Hit",
                        ],
                        title=f"Flash Hit and DRAM Hit by {category}",
                        yaxis_title="Hit",
                        xaxis_title=category,
                        mode="stack",
                    ),
                )
                tmp = tmp.sort_values(by=category)
                writer.Write("##### Detail Table  \n")
                writer.Write(
                    tabulate(
                        tmp,
                        headers="keys",
                        tablefmt="html",
                        showindex="never",
                        intfmt=",",
                    )
                    + "  \n\n",
                )


def WriteSumz(
    df: pd.DataFrame,
    ignore_obj_size: bool,
    category: str,
    numeric_modifier_continuous: str,
    numeric_modifier_spesific: tuple[str, float],
    title: str,
):
    current_title = f"{title} Categorized By {category} with {numeric_modifier_spesific[0]}: {numeric_modifier_spesific[1]}"
    html_path = os.path.join(
        OUTPUT_PATH,
        f"{'ignore_obj_size' if ignore_obj_size else 'not_ignore_object_size'}/{current_title}.html",
    )
    md_path = os.path.join(
        OUTPUT_PATH,
        f"../../markdown/{'ignore_obj_size' if ignore_obj_size else 'not_ignore_object_size'}/{current_title}.md",
    )
    writer = DocsWriter(html_path=html_path, md_path=None)
    WriteIndividualV2(
        writer,
        df.query(f"`{numeric_modifier_spesific[0]}` == @numeric_modifier_spesific[1]"),
        current_title,
        category,
        numeric_modifier_continuous,
    )
    writer.Flush()
    print("Finished generating " + current_title)


def Sumz(files: list[str], title: str, ignore_obj_size: bool = True, use_cache=True):
    files = [f for f in files if ("ignore_obj_size" in f) == ignore_obj_size]
    combined: pd.DataFrame
    cache = f".cache/{title}.pkl"
    os.makedirs(".cache", exist_ok=True)
    if use_cache and Path(cache).exists():
        print("Using cached DataFrame")
        with open(cache, "rb") as c:
            combined = pickle.load(c)
    else:
        print(f"Processing DataFrame {title}")
        offline_clock = GetOfflineClockResult(
            [f for f in files if "offline-clock" in f]
        )
        fifo = GetOtherResult([f for f in files if ",fifo," in f], "FIFO")
        lru = GetOtherResult([f for f in files if ",lru," in f], "LRU")
        combined = pd.concat([offline_clock, fifo, lru])
        if combined.empty:
            print(f"Title: {title}")
            print(f"ignore_obj_size: {ignore_obj_size}")
            print("is Empty")
            return
        with open(cache, "wb") as c:
            pickle.dump(combined, c)

    modifier = ["DRAM Size", "Flash Admission Treshold"]
    modifier_permutations = list(itertools.permutations(modifier, 2))
    args = []
    group_cols = ["Admission Threshold Method", "DRAM Algorithm", "Admissioner"]
    for (threshold_method, dram_algo, admissioner), df in combined.groupby(group_cols):
        for a, b in modifier_permutations:
            for i in df[a].unique():
                args.append(
                    (
                        df,
                        ignore_obj_size,
                        "Algorithm",
                        b,
                        (a, i),
                        f"threshold_method={threshold_method}/dram={dram_algo}/admissioner={admissioner}/{title}",
                    )
                )

    max_core = int(sys.argv[1]) if len(sys.argv) > 1 else None
    pprint("Generating figures with " + str(max_core) + " cores")
    with multiprocessing.Pool(max_core) as pool:
        pool.starmap(WriteSumz, args)


def FilterByTraceGroups(trace_groups: list[str]):
    log_path = os.path.join(DATA_PATH, "log")
    files = sorted(glob(os.path.join(log_path, "*.json")), key=sort_key)

    trace_list = []
    for trace_group in trace_groups:
        trace_list_file = open(f"../../trace/{trace_group}.txt", "r")
        trace_list = trace_list_file.readlines()
        trace_list_file.close()

    trace_list = [os.path.basename(t).strip() for t in trace_list]
    trace_list = [t for t in trace_list if t != ""]
    trace_list = [t[: t.find(".oracleGeneral")] for t in trace_list]

    return [f for f in files if os.path.basename(f[: f.find("[")]) in trace_list]


def main():
    zipf = FilterByTraceGroups(["zipf"])
    wiki = FilterByTraceGroups(["wiki_small"])
    metacdn = FilterByTraceGroups(["metacdn"])
    cloudphysics = FilterByTraceGroups(["cloudphysics"])
    tencentphoto = FilterByTraceGroups(["tencentphoto"])

    use_cache = False

    zipf = [x for x in zipf if "dram=none" in x]
    cloudphysics = [x for x in cloudphysics if "dram=none" in x]
    metacdn = [x for x in metacdn if "dram=none" in x]

    zipf = [x for x in zipf if "admissioner" not in x]
    cloudphysics = [x for x in cloudphysics if "admissioner" not in x]
    metacdn = [x for x in metacdn if "admissioner" not in x]

    args = [
        (zipf, "Zipf", False, use_cache),
        (cloudphysics, "CloudPhysics", False, use_cache),
        (metacdn, "MetaCDN", False, use_cache),
        # (wiki, "Wiki", False, use_cache),
        # (tencentphoto, "TencentPhotos", False, use_cache),
        # (zipf, "Zipf", True, use_cache),
        # (cloudphysics, "CloudPhysics", True, use_cache),
        # (metacdn, "MetaCDN", True, use_cache),
        # (wiki, "Wiki", True, use_cache),
        # (tencentphoto, "TencentPhotos", True, use_cache),
    ]

    processes = []
    for arg in args:
        proc = multiprocessing.Process(target=Sumz, args=arg)
        processes.append(proc)
        proc.start()
        pprint(f"Started process for {arg[1]} with ignore_obj_size={arg[2]}")

    for proc in processes:
        proc.join()


if __name__ == "__main__":
    main()
