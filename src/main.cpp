// Shared data
struct LaneStats {
  int count;
  float flow;
  float avgSpeed;
};
LaneStats laneA, laneB, laneC;

// Global timing defaults
unsigned long greenA = 20, greenB = 20, greenC = 20;
unsigned long overlap = 5;

void loop() {
  // Step through traffic light state machine
  trafficController(greenA, greenB, greenC, overlap);

  // While A is green → run UltrasonicSensor(U1)
  if (currentStep == 0) UltrasonicSensor("U1", sensor1, greenA*1000, 5, 32);
  // While B is green → run UltrasonicSensor(U2)
  if (currentStep == 2) UltrasonicSensor("U2", sensor2, greenB*1000, 23, 18);
  // While C is green → run UltrasonicSensor(U3)
  if (currentStep == 4) UltrasonicSensor("U3", sensor3, greenC*1000, 27, 34);

  // When a green phase ends → capture stats into LaneStats
  // Example: after Step 0 finishes, assign sensor1 results → laneA
}
