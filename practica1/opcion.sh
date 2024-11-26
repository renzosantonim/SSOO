unico=
lista=
leelista=0
leeProg=0
listaProg=

usage() { 
    echo "opciones.sh [-u arg] [-l arg ...] [prog [arg1 ...]]"
}

while [ -n "$1" ]; do

    case "$1" in
        -u )
            leelista=0;
	    if [ "$leeProg" -eq 1 ]; then
		leeProg=2
	    fi
            shift
            if [ -z "$1" ]; then
                usage
                exit 1
            fi
            unico="$1"
            ;;
        -l )
            leelista=1;
	    if [ "$leeProg" -eq 1 ]; then
		leeProg=2
	    fi
            ;;
        * ) if [ "$leelista" -ne 1 -a "$leeProg" -ne 2 ]; then
		leeProg=1
		listaProg+="$1 "
            elif [ "$leelista" -eq 1 ]; then
                lista+="$1 "
            else
                usage
                exit 1
            fi
	    ;;
     esac
     shift
done

if [ -n "$unico" ]; then
    echo "Unico es $unico"
fi

if [ -n "$lista" ]; then
    echo "Lista es $lista"
fi

if [ -n "$listaProg" ]; then
	echo "Lista de programa es $listaProg"
fi
