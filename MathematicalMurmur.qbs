import qbs.FileInfo
import qbs.ModUtils
import qbs.TextFile
import qbs.Utilities
import qbs.Xml

Project {
	QtApplication {
		name: "MathemticalMurmur"

		cpp.cppFlags: ['-Werror=return-type']
		cpp.cxxLanguageVersion: "c++20"

		files: ["**.cpp"]

		Depends { name: "Qt"; submodules: ["gui", "concurrent", "widgets", "quick", "quickcontrols2", "qml"] }
	}
}
