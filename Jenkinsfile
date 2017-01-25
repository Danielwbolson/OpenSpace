stage('Build') {
	parallel linux: {
		node('linux') {
			checkout scm
			sh 'git submodule update --init --recursive'
			sh 'mkdir -p build && cd build && python ../support/jenkins/buildAllModules.py && make'
		}
	},
	osx: {
		node('osx') {
			checkout scm
			sh '''
				export PATH=${PATH}:/usr/local/bin:/Applications/CMake.app/Contents/bin
				export CMAKE_BUILD_TOOL=/Applications/CMake.app/Contents/bin/CMake
				srcDir=$PWD
				if [ ! -d ${srcDir} ]; then
				  mkdir ${srcDir}
				fi
				if [ ! -d ${srcDir}/build ]; then
				  mkdir ${srcDir}/build
				fi
				cd ${srcDir}/build
				/Applications/CMake.app/Contents/bin/cmake -G Xcode -D NASM=/usr/local/Cellar/nasm/2.11.08/bin/nasm ${srcDir}
				xcodebuild
		 	'''
		}
	}
}	