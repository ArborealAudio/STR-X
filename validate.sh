#!/bin/bash

set -e

plugin=STR-X

# Install pluginval
# NOTE: We only validate VST3 because AU seems to not work on CI. Keeping plugin_path
# as array in case we add more paths later.
# Don't really care about validating VST2 and we'd have to pull in the SDK to
# build it anyway

plugin_path=("build/${plugin}_artefacts/Release/VST3/${plugin}.vst3")

if [[ $RUNNER_OS == 'Windows' ]]; then
	powershell -Command "[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12; Invoke-WebRequest https://github.com/Tracktion/pluginval/releases/download/v1.0.3/pluginval_Windows.zip -OutFile pluginval.zip"
	powershell -Command "Expand-Archive -Path ./pluginval.zip -DestinationPath ."
	pluginval="./pluginval.exe"
else
	wget -O pluginval.zip https://github.com/Tracktion/pluginval/releases/download/v1.0.3/pluginval_${RUNNER_OS}.zip
	unzip pluginval
	if [[ $RUNNER_OS == 'macOS' ]]; then
		pluginval="pluginval.app/Contents/MacOS/pluginval"
	else
		pluginval="./pluginval"
	fi
fi

for p in ${plugin_path[@]}; do
	echo "Validating $p"
	if $pluginval --strictness-level 10 --validate-in-process --skip-gui-tests --timeout-ms 300000 $p;
	then
		echo "Pluginval successful"
	else
		echo "Pluginval failed"
		rm -rf pluginval*
		exit 1
	fi
done

rm -rf pluginval*
