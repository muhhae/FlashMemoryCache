import multiprocessing
import os
from os.path import exists
import pickle
from glob import glob
from pathlib import Path
from pprint import pprint

import pandas as pd
from pandas.core import algorithms
from pandas.core.frame import itertools
from common import sort_key
from data_reader_json import GetOfflineClockResult, GetOtherResult
from docs_writer import Write, WriteFig, WriteHTML
from plotly.graph_objs import Figure, Trace
from plotly_wrapper import Line, Scatter, VerticalCompositionBar
from tabulate import tabulate


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


def WriteMean(md, html, df: pd.DataFrame):
    Write(md, html, "# Mean  \n")
    for s in df["Cache Size"].unique():
        Write(md, html, f"## {s}  \n")
        data = (
            df.query("`Cache Size` == @s")
            .groupby("Model")[["Miss Ratio", "Flash Write", "Promotion", "Miss"]]
            .mean()
            .reset_index()
            .sort_values(by="Flash Write")
        )
        fig = Scatter(
            data,
            x="Flash Write",
            y="Miss Ratio",
            color="Model",
            symbol="Model",
        )
        WriteFig(md, html, fig)
        fig = CreateFlashWriteComposition(data)
        WriteFig(md, html, fig)
        Write(
            md,
            html,
            tabulate(
                data[["Model", "Miss Ratio", "Miss", "Promotion", "Flash Write"]],
                headers=[
                    "Algorithm",
                    "Miss Ratio",
                    "Cache Miss",
                    "Reinsertion",
                    "Flash Write",
                ],
                tablefmt="html",
                showindex="never",
                intfmt=",",
            )
            + "  \n\n",
        )


def WriteMeanReduction(md, html, df: pd.DataFrame):
    Write(md, html, "# Mean Reduction Compared to FIFO  \n")
    for s in df["Cache Size"].unique():
        Write(md, html, f"## {s}  \n")
        data = (
            df.query("`Cache Size` == @s")
            .groupby("Model")[
                [
                    "Miss Ratio Reduction",
                    "Flash Write Reduction",
                    "Miss Reduction",
                    "Promotion Reduction",
                ]
            ]
            .mean()
            .reset_index()
            .sort_values(by="Flash Write Reduction", ascending=False)
        )

        fig = Scatter(
            data,
            x="Flash Write Reduction",
            y="Miss Ratio Reduction",
            color="Model",
            symbol="Model",
        )
        WriteFig(md, html, fig)
        Write(
            md,
            html,
            tabulate(
                data[
                    [
                        "Model",
                        "Miss Ratio Reduction",
                        "Miss Reduction",
                        "Promotion Reduction",
                        "Flash Write Reduction",
                    ]
                ],
                headers=[
                    "Algorithm",
                    "Miss Ratio Reduction",
                    "Cache Miss Reduction",
                    "Reinsertion Reduction",
                    "Flash Write Reduction",
                ],
                tablefmt="html",
                showindex="never",
                intfmt=",",
            )
            + "  \n\n",
        )


def WriteIndividualReduction(md, html, df: pd.DataFrame):
    Write(md, html, "# Individual Reduction Result  \n")
    for s in df["Cache Size"].unique():
        Write(md, html, f"## {s}  \n")
        for t in df["Trace"].unique():
            Write(md, html, f"### {Path(t).stem}  \n")
            data = df.query("`Cache Size` == @s and `Trace` == @t").sort_values(
                by="Flash Write Reduction", ascending=False
            )
            fig = Scatter(
                data,
                x="Flash Write Reduction",
                y="Miss Ratio Reduction",
                color="Model",
                symbol="Model",
            )
            WriteFig(md, html, fig)
            Write(
                md,
                html,
                tabulate(
                    data[
                        [
                            "Model",
                            "Miss Ratio Reduction",
                            "Miss Reduction",
                            "Promotion Reduction",
                            "Flash Write Reduction",
                            "JSON File",
                        ]
                    ],
                    headers=[
                        "Algorithm",
                        "Miss Ratio Reduction",
                        "Cache Miss Reduction",
                        "Reinsertion Reduction",
                        "Flash Write Reduction",
                    ],
                    tablefmt="html",
                    showindex="never",
                    intfmt=",",
                )
                + "  \n\n",
            )


def WriteIndividual(
    md,
    html,
    df: pd.DataFrame,
    category: str,
    add_desc: str = "",
):
    Write(md, html, f"# Individual Result {add_desc} \n")
    for s in sorted(df["Trace"].unique()):
        Write(md, html, f"## {s}  \n")
        for t in sorted(df["Cache Size"].unique()):
            Write(md, html, f"### {t * 100}%  \n")
            data = df.query("`Cache Size` == @t and `Trace` == @s").sort_values(
                by="Write"
            )
            data = data.drop_duplicates(subset=[category])
            # Write(md, html, f"#### Overall Miss Ratio with {category}  \n")
            # WriteFig(
            #     md,
            #     html,
            #     Line(
            #         data,
            #         x=category,
            #         y="Overall Miss Ratio",
            #     ),
            # )
            # for y in ["Overall Miss Ratio", "Flash Miss Ratio"]:
            #     Write(md, html, f"#### {y}  \n")
            #     WriteFig(
            #         md,
            #         html,
            #         Scatter(
            #             data,
            #             x="Write",
            #             y=y,
            #             color=category,
            #             symbol=category,
            #         ),
            #     )
            #     Write(md, html, f"#### {y} Absolute  \n")
            #     WriteFig(
            #         md,
            #         html,
            #         Scatter(
            #             data,
            #             include_zero=True,
            #             x="Write",
            #             y=y,
            #             color=category,
            #             symbol=category,
            #         ),
            #     )
            Write(md, html, "#### Inserted + Reinserted  \n")
            WriteFig(
                md,
                html,
                VerticalCompositionBar(
                    data,
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
            Write(md, html, "#### Flash Hit and DRAM Hit  \n")
            WriteFig(
                md,
                html,
                VerticalCompositionBar(
                    data,
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
            Write(md, html, "#### Detail Table  \n")
            Write(
                md,
                html,
                tabulate(
                    data[
                        [
                            "Algorithm",
                            "Flash Admission Treshold",
                            "Overall Request",
                            "Flash Request",
                            "DRAM Request",
                            "Overall Miss Ratio",
                            "Flash Miss Ratio",
                            "DRAM Miss Ratio",
                            "Overall Hit",
                            "Flash Hit",
                            "DRAM Hit",
                            "Write",
                            "JSON File",
                        ]
                    ],
                    headers="keys",
                    tablefmt="html",
                    showindex="never",
                    intfmt=",",
                )
                + "  \n\n",
            )


def WriteIndividualV2(
    md,
    html,
    df: pd.DataFrame,
    add_desc: str,
    category: str,
    numeric_modifier_continuous: str,
):
    Write(md, html, f"# Individual Result {add_desc} \n")
    for s in sorted(df["Trace"].unique()):
        Write(md, html, f"## {s}  \n")
        for t in sorted(df["Cache Size"].unique()):
            Write(md, html, f"### {t * 100}%  \n")
            data = df.query("`Cache Size` == @t and `Trace` == @s").sort_values(
                by=numeric_modifier_continuous,
            )
            for y in [
                "Overall Miss Ratio",
                "Flash Miss Ratio",
                "Write",
                "Inserted",
                "Reinserted",
            ]:
                Write(
                    md,
                    html,
                    f"#### Effects of {numeric_modifier_continuous} on {y}  \n",
                )
                WriteFig(
                    md,
                    html,
                    Line(data, x=numeric_modifier_continuous, y=y, color=category),
                )
            for i in sorted(df["numeric_modifier_continuous"].unique()):
                pass
            data = data.sort_values(by=category)
            Write(md, html, "#### Detail Table  \n")
            Write(
                md,
                html,
                tabulate(
                    data[
                        [
                            "Algorithm",
                            "Flash Admission Treshold",
                            "Overall Request",
                            "Flash Request",
                            "DRAM Request",
                            "Overall Miss Ratio",
                            "Flash Miss Ratio",
                            "DRAM Miss Ratio",
                            "Overall Hit",
                            "Flash Hit",
                            "DRAM Hit",
                            "Write",
                            "JSON File",
                        ]
                    ],
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
    html = open(
        f"../../docs/{'ignore_obj_size' if ignore_obj_size else 'not_ignore_object_size'}/{current_title}.html",
        "w",
    )
    md = open(
        f"../../markdown/{'ignore_obj_size' if ignore_obj_size else 'not_ignore_object_size'}/{current_title}.md",
        "w",
    )
    WriteIndividualV2(
        md,
        html,
        df.query(f"`{numeric_modifier_spesific[0]}` == @numeric_modifier_spesific[1]"),
        current_title,
        category,
        numeric_modifier_continuous,
    )
    WriteHTML(html)
    md.close()
    html.close()
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
        fifo = GetOtherResult(files, "FIFO", "fifo")
        lru = GetOtherResult(files, "LRU", "lru")
        combined = pd.concat([offline_clock, fifo, lru])
        if combined.empty:
            print(f"Title: {title}")
            print(f"ignore_obj_size: {ignore_obj_size}")
            print("is Empty")
            return
        # pprint(combined.iloc[0]["Flash Metrics Time"])
        # pprint(combined.iloc[0]["JSON File"])
        # exit(1)
        with open(cache, "wb") as c:
            pickle.dump(combined, c)

    os.makedirs(
        f"../../docs/{'ignore_obj_size' if ignore_obj_size else 'not_ignore_object_size'}/",
        exist_ok=True,
    )
    os.makedirs(
        f"../../markdown/{'ignore_obj_size' if ignore_obj_size else 'not_ignore_object_size'}/",
        exist_ok=True,
    )

    modifier = ["DRAM Size", "Flash Admission Treshold"]
    modifier_permutations = list(itertools.permutations(modifier, len(modifier)))
    args = []
    for a, b in modifier_permutations:
        for i in combined[a].unique():
            args.append((combined, ignore_obj_size, "Algorithm", b, (a, i), title))

    with multiprocessing.Pool(4) as pool:
        pool.starmap(WriteSumz, args)


def FilterByTraceGroups(trace_groups: list[str]):
    log_path = "../../../FlashMemoryCacheResults/log/"
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

    Sumz(zipf, "Zipf", False, use_cache)
    Sumz(cloudphysics, "CloudPhysics", False, use_cache)
    Sumz(metacdn, "MetaCDN", False, use_cache)
    Sumz(wiki, "Wiki", False, use_cache)
    Sumz(tencentphoto, "TencentPhotos", False, use_cache)

    Sumz(zipf, "Zipf", True, use_cache)
    Sumz(cloudphysics, "CloudPhysics", True, use_cache)
    Sumz(metacdn, "MetaCDN", True, use_cache)
    Sumz(wiki, "Wiki", True, use_cache)
    Sumz(tencentphoto, "TencentPhotos", True, use_cache)


if __name__ == "__main__":
    main()
