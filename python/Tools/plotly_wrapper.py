from typing import List

import pandas as pd
import plotly.express as px
import plotly.graph_objs as go
from plotly.io import templates

templates.default = "plotly_white"


def Line(
    df: pd.DataFrame, x, y, size=False, count=1, include_zero: bool = False, **kwargs
) -> go.Figure:
    fig = px.line(
        df,
        x=x,
        y=y,
        **kwargs,
    )
    fig.update_layout(
        font=dict(size=32),
        height=750 * count,
        width=1000,
    )
    fig.update_traces(
        opacity=0.5,
        line=dict(width=3),
    )
    fig.update_xaxes(showgrid=True, nticks=12)
    fig.update_yaxes(showgrid=True, nticks=18)
    if size:
        fig.update_layout(yaxis_tickformat="s")

    if include_zero:
        fig.update_xaxes(rangemode="tozero")
        fig.update_yaxes(range=[0, 1])
    return fig


def Scatter(df: pd.DataFrame, include_zero: bool = False, **kwargs) -> go.Figure:
    fig = px.scatter(df, **kwargs)
    fig.update_layout(
        font=dict(size=32),
        # height=750,
        width=750,
    )
    fig.update_traces(
        marker_size=36,
        marker_line=dict(width=6),
        selector=dict(mode="markers"),
    )
    fig.update_xaxes(showgrid=True, nticks=10)
    fig.update_yaxes(showgrid=True, nticks=30)
    if include_zero:
        fig.update_xaxes(rangemode="tozero")
        fig.update_yaxes(range=[0, 1])
    return fig


def Box(df: pd.DataFrame, **kwargs) -> go.Figure:
    fig = px.box(df, **kwargs)
    fig.update_xaxes(dtick=10)
    fig.update_layout(
        font=dict(size=32),
        height=30 * len(df[kwargs.get("y")].unique()),
        width=750,
        showlegend=False,
    )
    fig.update_xaxes(showgrid=True, nticks=10)
    fig.update_yaxes(showgrid=True, nticks=20)
    return fig


def VerticalCompositionBar(
    df: pd.DataFrame,
    X: str,
    Ys: List[str | tuple[str, str]],
    title: str | None = None,
    yaxis_title: str | None = None,
    xaxis_title: str | None = None,
    mode: str = "stack",
    y_max=None,
) -> go.Figure:
    df = df.sort_values(by=X)
    fig = go.Figure()
    for Y in Ys:
        fig.add_trace(
            go.Bar(
                x=df[X],
                y=df[Y] if isinstance(Y, str) else df[Y[0]],
                name=Y if isinstance(Y, str) else Y[1],
            )
        )
    fig.update_layout(
        barmode=mode,
        font=dict(size=32),
        title_text=title,
        xaxis_title=xaxis_title,
        yaxis_title=yaxis_title,
    )
    fig.update_xaxes(showgrid=True, nticks=10)
    fig.update_yaxes(showgrid=True, nticks=10, range=[0, y_max])
    fig.update_xaxes(type="category")
    return fig
