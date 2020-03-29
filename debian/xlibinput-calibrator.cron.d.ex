#
# Regular cron jobs for the xlibinput-calibrator package
#
0 4	* * *	root	[ -x /usr/bin/xlibinput-calibrator_maintenance ] && /usr/bin/xlibinput-calibrator_maintenance
