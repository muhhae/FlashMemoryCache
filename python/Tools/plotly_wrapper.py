from typing import List

import pandas as pd
import plotly.express as px
import plotly.graph_objs as go
from plotly.io import templates

templates.default = "plotly_dark"


def Scatter(df: pd.DataFrame, **kwargs) -> go.Figure:
    fig = px.scatter(df, **kwargs)
    fig.update_layout(
        font=dict(size=24),
        height=800,
        width=1000,
    )
    fig.update_traces(
        marker_size=24,
        marker_line=dict(width=6),
        selector=dict(mode="markers"),
    )
    fig.update_xaxes(showgrid=True, nticks=20)
    fig.update_yaxes(showgrid=True, nticks=20)
    return fig


def Box(df: pd.DataFrame, **kwargs) -> go.Figure:
    fig = px.box(df, **kwargs)
    fig.update_xaxes(dtick=10)
    fig.update_layout(
        font=dict(size=24),
        height=30 * len(df[kwargs.get("y")].unique()),
        width=1000,
        showlegend=False,
    )
    fig.update_xaxes(showgrid=True, nticks=20)
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
) -> go.Figure:
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
        font=dict(size=24),
        title_text=title,
        xaxis_title=xaxis_title,
        yaxis_title=yaxis_title,
    )
    fig.update_xaxes(showgrid=True, nticks=10)
    fig.update_yaxes(showgrid=True, nticks=10)
    fig.update_xaxes(type="category")
    return fig
