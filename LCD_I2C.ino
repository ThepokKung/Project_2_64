void lcd_Show_16_2(int pm1,int pm2_5,int dtemp,int dhum){
  lcd.clear();
  lcd.print("PM1:");
  lcd.setCursor(4,0);
  lcd.print(pm1);
  lcd.setCursor(7,0);
  lcd.print("PM2_5:");
  lcd.setCursor(13,0);
  lcd.print(pm2_5);
  lcd.setCursor(0,1);
  lcd.print("DTemp:");
  lcd.setCursor(6,1);
  lcd.print(dtemp);
  lcd.setCursor(9,1);
  lcd.print("DHum:");
  lcd.setCursor(14,1);
  lcd.print(dhum);
}
