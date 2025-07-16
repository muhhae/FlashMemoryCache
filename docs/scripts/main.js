document.addEventListener("DOMContentLoaded", function () {
  const sidebar = document.getElementById("sidebar");
  const btn = document.getElementById("sidebarToggleBtn");
  const content = document.querySelector(".content");

  sidebar.classList.add("closed");

  btn.addEventListener("click", function () {
    sidebar.classList.toggle("closed");
    if (sidebar.classList.contains("closed")) {
      btn.setAttribute("aria-label", "Open sidebar");
    } else {
      btn.setAttribute("aria-label", "Close sidebar");
    }
  });

  if (window.innerWidth <= 700) {
    sidebar.classList.add("closed");
  }

  document.querySelectorAll(".sidenav li").forEach((item) => {
    const sublist = item.querySelector("ul");
    if (sublist) {
      const link = item.querySelector("a");
      if (link) {
        link.classList.add("collapsible");
        link.addEventListener("click", (e) => {
          if (link.getAttribute("href") === "#") {
            e.preventDefault();
          }
          item.classList.toggle("open");
        });
      }
    }
  });

  const chartContainers = document.querySelectorAll(".chart-placeholder");
  const loadedCharts = new Set();

  const observer = new IntersectionObserver(
    (entries, observer) => {
      entries.forEach((entry) => {
        const container = entry.target;
        const chartId = container.id;

        if (entry.isIntersecting) {
          if (!loadedCharts.has(chartId)) {
            container.classList.add("loading");

            const chartData = JSON.parse(container.dataset.chartJson);
            console.log("Parsed chartData:", chartData); // <--- THIS IS THE MOST IMPORTANT LOG
            console.log("chartData.data:", chartData.data); // <--- ADD THIS
            console.log("chartData.layout:", chartData.layout);

            if (typeof Plotly !== "undefined") {
              console.log("Loading container:", container);
              container.textContent = "";
              Plotly.newPlot(container, chartData.data, chartData.layout);
              loadedCharts.add(chartId);
              container.classList.remove("loading");
            } else {
              console.warn("Plotly.js not loaded. Chart will not render.");
            }
          }
        } else {
          if (loadedCharts.has(chartId)) {
            if (typeof Plotly !== "undefined") {
              console.log(`Unloading chart: ${chartId}`);
              Plotly.purge(container);
              loadedCharts.delete(chartId);
              container.innerHTML = "Chart placeholder (scroll back to load)";
            }
          }
        }
      });
    },
    {
      rootMargin: "100px",
      threshold: [0, 0.01],
    },
  );

  chartContainers.forEach((container) => {
    observer.observe(container);
  });
});
