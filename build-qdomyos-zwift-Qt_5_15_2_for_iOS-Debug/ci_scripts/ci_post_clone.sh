brew install qt@5
sed -i '' '/5E618435888B9D49F8540165 \/\* Qt Qmake \*\//d' /Volumes/workspace/repository/build-qdomyos-zwift-Qt_5_15_2_for_iOS-Debug/qdomyoszwift.xcodeproj
sed -i '' 's|/Users/cagnulein/qdomyos-zwift|..|g' /Volumes/workspace/repository/build-qdomyos-zwift-Qt_5_15_2_for_iOS-Debug/qdomyoszwift.xcodeproj
sed -i '' 's|/Users/cagnulein/Qt/5.15.2/ios/|/usr/local/opt/qt@5/|g' /Volumes/workspace/repository/build-qdomyos-zwift-Qt_5_15_2_for_iOS-Debug/qdomyoszwift.xcodeproj
