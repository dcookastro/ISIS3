APPNAME = dawnvir2isis

include $(ISISROOT)/make/isismake.tsts

#  The sed command is use to delete the SourceFile keyword from the label which
#  can span two lines.
commands:
	$(APPNAME) from=$(INPUT)/VIR_VIS_1A_1_362681635_1.LBL \
         to=$(OUTPUT)/VIR_VIS_1A_1_362681635_1.cub > /dev/null;
	catlab from=$(OUTPUT)/VIR_VIS_1A_1_362681635_1.cub  \
	  to=$(OUTPUT)/labels.pvl > /dev/null;
	tabledump from=$(OUTPUT)/VIR_VIS_1A_1_362681635_1.cub \
	  to=$(OUTPUT)/table.txt name=VIRHouseKeeping > /dev/null;
