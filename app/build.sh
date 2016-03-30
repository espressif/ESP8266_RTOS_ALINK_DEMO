#export SDK_PATH=/home/esp8266/Share/esp_iot_sdk_freertos
#export BIN_PATH=$SDK_PATH/bin

build_code() {

	#make BOOT=none APP=0 SPI_SPEED=20 SPI_MODE=QIC SPI_SIZE_MAP=4
	make BOOT=new APP=1 SPI_SPEED=40 SPI_MODE=QIC SPI_SIZE_MAP=5
}
if [ -z "$1" ]; then
make clean
#make BOOT=none APP=0 SPI_SPEED=20 SPI_MODE=QIC SPI_SIZE_MAP=4
build_code
fi


if [ '-alink' == "-$1" ]; then
	cd ../alink_sdk
	make clean
	build_code
	#make BOOT=none APP=0 SPI_SPEED=20 SPI_MODE=QIC SPI_SIZE_MAP=4
	cd -
	cp -rf ../alink_sdk/user/.output/eagle/debug/lib/libalink.a  ../lib/
	if [ $? != 0 ];then
		echo ""
		echo ""
		echo "build libalink.a error!"
	fi
elif [ '-c' == "-$1" ]; then
	cd ../alink_sdk
	cp ./user/.output/eagle/debug/lib/libalink.a  ../lib/
	cd -
elif [ '-ssl' == "-$1" ]; then
	cd ../third_party/ssl
	make clean
	build_code
	#make BOOT=none APP=0 SPI_SPEED=20 SPI_MODE=QIC SPI_SIZE_MAP=4
	cd -
	cp -rf ../third_party/ssl/.output/eagle/debug/lib/libssl.a  ../lib/
	
	if [ $? != 0 ];then
		echo ""
		echo ""
		echo "build libssl.a error!"
	fi

elif [ '-a' == "-$1" ]; then
	./build.sh ssl;
	if [ -f ../third_party/ssl/.output/eagle/debug/lib/libssl.a ];then
		./build.sh alink;
		if [ -f  ../alink_sdk/user/.output/eagle/debug/lib/libalink.a ];then
			./build.sh
		else
			echo "build alink sdk error!"
		fi
	else
		echo "build ssl error!"	
	fi

elif [ '-b' == "-$1" ]; then
	./build.sh alink
	if [ -f  ../alink_sdk/user/.output/eagle/debug/lib/libalink.a ];then
		./build.sh
	else
		echo ""
		echo ""
		echo "build libalink.a error!"
	fi

fi
