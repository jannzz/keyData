const refreshButton = document.getElementById("refreshButton");
const deleteNetworkBtn = document.getElementById("deleteNetworkBtn");
const deleteDataLogBtn = document.getElementById("deleteDataLogBtn");

refreshButton.addEventListener("click", function () {
  window.location.reload();
});

deleteNetworkBtn.addEventListener("click", function () {
  deleteNetworkSetting();
});

deleteDataLogBtn.addEventListener("click", function () {
  deleteDataLog();
});

function getData() {
  fetch("/log")
    .then((response) => response.json())
    .then((data) => {
      const states = [];
      const timestamps = [];
      data.forEach(item => {
        states.push(item.state);
        timestamps.push(item.timestamp);
      });
      Highcharts.chart("chart-states", {
        chart: {
          backgroundColor: "#333333",
          type: "line",
        },
        title: {
          text: "RFID State Changes",
          style: {
            color: "#E0E0E0",
          },
        },
        xAxis: {
          categories: timestamps,
          title: {
            text: "Timestamp",
            style: {
              color: "#E0E0E0",
            },
          },
          labels: {
            style: {
              color: "#E0E0E0",
            },
          },
        },
        yAxis: {
          title: {
            text: "State",
            style: {
              color: "#E0E0E0",
            },
          },
          labels: {
            style: {
              color: "#E0E0E0",
            },
          },
          categories: ['Key is not touching', 'Key touching', 'Open lock'],
          tickInterval: 1
        },
        tooltip: {
          formatter: function () {
            return '<b>' + this.x + '</b><br/>' + this.y;
          },
          backgroundColor: '#000000',
          borderColor: '#FFFFFF',
          style: {
            color: '#FFFFFF'
          }
        },
        plotOptions: {
          line: {
            marker: {
              radius: 4,
              lineWidth: 1,
              lineColor: "#FFFFFF",
              fillColor: "#FF0000",
              symbol: "circle",
            },
            lineWidth: 2,
            color: "#FF0000"
          },
        },
        series: [
          {
            name: "State",
            data: states.map(state => {
              if (state === "Key is not touching") return 0;
              if (state === "Key touching") return 1;
              if (state === "Open lock") return 2;
              return -1;
            }),
            color: "#00FF00",
          },
        ],
        legend: {
          itemStyle: {
            color: "#E0E0E0",
          },
        },
      });
    })
    .catch((error) => {
      console.error("Error processing data:", error);
    });
}

function deleteNetworkSetting() {
  fetch("/deleteNetwork")
    .then((data) => {
      console.log("Data deleted:", data);
      window.location.reload();
    })
    .catch((error) => {
      console.error("Error processing data:", error);
    });
  console.log("Data deleted");
}

function deleteDataLog() {
  fetch("/deleteDataLog")
    .then((data) => {
      console.log("Data deleted:", data);
      window.location.reload();
    })
    .catch((error) => {
      console.error("Error processing data:", error);
    });
  console.log("Data deleted");
  window.location.reload();
}

window.onload = getData;
