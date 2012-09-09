# Copies the given files to the destination directory
defineTest(copyToDestdir) {
	ddir  = $$1
	files = $$2

	system($$QMAKE_MKDIR $$quote($$ddir))

	for(FILE, files) {
		d = $$sprintf("%1/%2", $$ddir, $$dirname(FILE))
		system($$QMAKE_MKDIR $$quote($$d))
		system($$QMAKE_COPY $$quote($$FILE) $$quote($$d))
	}
}

MB_BASE_DIR = $$PWD
MB_BUILD_DIR = $$PWD/build