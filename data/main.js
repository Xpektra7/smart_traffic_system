let step = '';
const lanes = document.querySelectorAll('.lane');


let aR = '';
let bR = '';
let cR = '';


lanes[0].style.border = aR;
lanes[1].style.border = bR;
lanes[2].style.border = cR;


async function updateDashboard() {
  try {
    const res = await fetch("/status");
    const data = await res.json();

    // update body attribute
    step =  data.currentStep;

    // update stats
async function updateDashboard() {
  try {
    const res = await fetch("/status");
    const data = await res.json();

    // update body attribute
    document.body.dataset.step = data.currentStep;

    // update per-lane stats
    const lanes = document.querySelectorAll(".lane");
    data.lanes.forEach((laneData, i) => {
      if (lanes[i]) {
        lanes[i].querySelector("p:nth-child(2) span").textContent = laneData.count;
        lanes[i].querySelector("p:nth-child(3) span").textContent = laneData.flow;
        lanes[i].querySelector("p:nth-child(4) span").textContent = laneData.speed;
      }
    });

  } catch (e) {
    console.error("Failed to update dashboard:", e);
  }
}

  } catch (e) {
    console.error("Failed to update dashboard:", e);
  }
}

// run every 100ms
setInterval(updateDashboard, 100);


switch (step) {
    case "0":
        aR = '3px solid yellow';
        bR = '3px solid red';
        cR = '3px solid red';
        break;
    case "1":
        aR = '3px solid yellow';
        bR = '3px solid yellow';
        cR = '3px solid red';
        break;
    case "2":
        aR = '3px solid red';
        bR = '3px solid green';
        cR = '3px solid red';
        break;
    case "3":
        aR = '3px solid red';
        bR = '3px solid yellow';
        cR = '3px solid yellow';
        break;
    case "4":
        aR = '3px solid red';
        bR = '3px solid red';
        cR = '3px solid green';
        break;
    case "5":
        aR = '3px solid red';
        bR = '3px solid yellow';
        cR = '3px solid yellow';
        break;

}

