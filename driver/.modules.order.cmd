cmd_/root/workspace/ssd1306/driver/modules.order := {   echo /root/workspace/ssd1306/driver/ssd1306.ko; :; } | awk '!x[$$0]++' - > /root/workspace/ssd1306/driver/modules.order
