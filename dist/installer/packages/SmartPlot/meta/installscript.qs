function Component()
{
     var programFiles = installer.environmentVariable("ProgramFiles");
     if (programFiles != "")
         installer.setValue("TargetDir", programFiles + "/SmartPlot_2.01.002");
}

Component.prototype.isDefault = function()
{
    // select the component by default
    return true;
}

Component.prototype.createOperations = function()
{
	component.createOperations();

	if (installer.value("os") === "win") {
		component.addOperation("CreateShortcut", "@TargetDir@/SmartPlot.exe", "@StartMenuDir@/SmartPlot.lnk",
			"workingDirectory=@TargetDir@", "iconLocation=@TargetDir@/smartplot.ico");
			
		component.addOperation("CreateShortcut", "@TargetDir@/uninstall.exe", "@StartMenuDir@/Uninstall.lnk",
			"workingDirectory=@TargetDir@", "iconLocation=@TargetDir@/smartplot.ico");
	}
}
