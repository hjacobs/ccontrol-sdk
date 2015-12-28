' Initializieren Beginn
  gosub LCD_INIT
  pause 10
' Initializieren Ende 
 

' Ausgabe am Display
  lcd_param = A_ : gosub LCD_WRITECHAR   
  lcd_param = 2 : gosub LCD_GOTOLINE     ' Wechsel in die zweite Zeile
  lcd_param = A_ : gosub LCD_WRITECHAR   

end

