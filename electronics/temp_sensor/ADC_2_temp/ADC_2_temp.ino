void setup() {
  pinMode(A0, INPUT);
  Serial.begin(9600);

}

void loop() {
  delay(500);
  int16_t adc_value = analogRead(A0);
  Serial.print(adc2temp(adc_value));
  Serial.print("\n");

}

float adc2temp(int16_t adc){
  float adc_f = (float)adc;
  float x4 = pow(adc_f, 4) * -2.864e-09;
  float x3 = pow(adc_f, 3) * 3.726e-06;
  float x2 = pow(adc_f, 2) * -0.00142;
  float x1 = adc_f * -0.01458;
  float x0 = 97.45;
  return x4 + x3 + x2 + x1 + x0;
}
