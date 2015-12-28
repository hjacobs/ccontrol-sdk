#MAINLOOP
    gosub KEY_IDENT
    pause 10
    if key_nr <> 255 then print key_nr
goto MAINLOOP

end

table key_codes "keyb.tab"    


