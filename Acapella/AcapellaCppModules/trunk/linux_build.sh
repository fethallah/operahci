find ./GNF_Modules -iname '*.cpp'|
perl -pe 's:\./(.+?)\.cpp:$1:'|
while
	read filename;
do
	g++ -Wall -fpic -D_GNU_SOURCE -MMD -fexceptions -pthread  -g -O2  -I./Libraries/AcapellaSDK2.7_Linux/include $filename.cpp -shared -Wl,-soname,$filename.so.2 -o lib$filename.so.2.7 -L/usr/local/PerkinElmerCTG/Acapella2.7.2/lib  -limacro -lbaseutil -lmemblock -lcells;
	rm lib$filename.so.2.d;
done