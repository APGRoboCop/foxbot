.PHONY: clean All

All:
	@echo "----------Building project:[ FoXBot - Win32_Release ]----------"
	@"$(MAKE)" -f  "FoXBot.mk"
clean:
	@echo "----------Cleaning project:[ FoXBot - Win32_Release ]----------"
	@"$(MAKE)" -f  "FoXBot.mk" clean
