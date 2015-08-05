static bool IsDst(int hour, int day, int month, int dow) {
  if (month < 3 || month > 10)  return false; 
  if (month > 3 && month < 10)  return true; 

  int previousSunday = day - dow;

//  if (month == 3) return previousSunday >= 25;
  if ((month == 3) && (previousSunday >= 25)) {
    return hour >= 2;
  }

//  if (month == 10) return previousSunday < 25;
  if ((month == 10) && (previousSunday <= 25)) {
    return hour < 2;
  }

  return false; // this line never gonna happend
}
