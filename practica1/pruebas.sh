#!/bin/bash

ordenamiento=user                                 # Por defecto, la tabla se ordena por nombre de usuario
lineas_sid0=0                                     # Si no se utiliza la opción -z, se excluyen los procesos con SID = 0
usuarios=()                                       # Array de usuarios
directorio=""                                     # Directorio del que se mirarán los procesos que abran archivos en él 
mostrar_terminal=0                                # Se activa al usar -t, para mostrar procesos con terminal controladora asociada
lista_procesos=0                                  # Se activa al usar -e, para mostrar la tabla de procesos (funcionamiento principal)

# Colores para cada columna
  sid_color="\033[31m"   # Rojo para SID
  pgid_color="\033[32m"  # Verde para PGID
  pid_color="\033[33m"   # Amarillo para PID
  user_color="\033[34m"  # Azul para USER
  tty_color="\033[35m"   # Magenta para TTY
  mem_color="\033[36m"   # Cian para %MEM
  cmd_color="\033[37m"   # Blanco para CMD
  reset_color="\033[0m"  # Resetear el color                             

usage() { 
    echo "infosession.sh [-h] [-z] [-u user1 ... ] [ -d dir ] [-t ]"
}

usuario_existe() {
  if ! id "$1" &>/dev/null; then                                                    # Si el comando id falla, significa que el usuario no existe
    echo "Error: El usuario '$1' no existe."
    exit 1
  fi
}

mostrar_tabla_procesos() 
{
  local ps_command="ps -eo sid,pgid,pid,user,tty,%mem,cmd --sort=$ordenamiento | tail -n+2 | tr -s ' '"      # Comando base para imprimir la tabla -> SID PGID PID USER TT %MEM CMD

  if [ ${#usuarios[@]} -gt 0 ]; then                                                # Filtro para múltiples usuarios. Tiene que haber al menos 1 usuario
    local users_pattern=$(IFS=\| ; echo "${usuarios[*]}")                           # Expresión regular que contiene a los usuarios separados por '|'
    ps_command="$ps_command | awk -v users=\"$users_pattern\" '\$4 ~ users'"        # Creamos en awk la variable con dicha expresión y filtramos la búsqueda con la columna user
  fi

  if [ $lineas_sid0 -eq 0 ]; then                                                   # Si no se utiliza la opción -z, se excluyen los procesos con SID = 0
    ps_command="$ps_command | awk '\$1 != 0'"
  fi 

  if [ -n "$directorio" ]; then                                                   # Si se proporciona un directorio con -d
    local pid_lsof=$(lsof +d "$directorio" 2>/dev/null | awk 'NR>1 {print $2}' | sort -u)   # Obtener los PIDs de los procesos que tienen archivos abiertos en el directorio dado
    if [ -n "$pid_lsof" ]; then                                                   # Verificar si pid_lsof contiene PIDs, y si es así, filtrarlos en el comando ps
      ps_command="$ps_command | awk '{if (index(\"$pid_lsof\", \$3)) print}'"
    fi
  fi

  if [ $mostrar_terminal -eq 1 ]; then                              # Filtrar solo procesos con TTY distinto de "?"
    ps_command="$ps_command | awk '\$5 != \"?\"'"
  fi

  # Imprimir la cabecera con los colores
  echo -e "${sid_color}SID${reset_color}\t${pgid_color}PGID${reset_color}\t${pid_color}PID${reset_color}\t${user_color}USER${reset_color}\t${tty_color}TTY${reset_color}" \
  "${mem_color}%MEM${reset_color}\t${cmd_color}CMD${reset_color}"

  # Ejecutar ps y aplicar los colores a cada columna
  eval $ps_command | awk -v sid_color="$sid_color" -v pgid_color="$pgid_color" -v pid_color="$pid_color" -v user_color="$user_color" \
  -v tty_color="$tty_color" -v mem_color="$mem_color" -v cmd_color="$cmd_color" -v reset_color="$reset_color" '
  {
    # Imprimir las columnas con colores
    print sid_color $1, pgid_color $2, pid_color $3, user_color $4, tty_color $5, mem_color $6, cmd_color $7, reset_color
  }' | column -t  # Alinear las columnas
}

mostrar_tabla_sesiones()
{
  echo "mostrar tabla de sesiones"
}

while [ -n "$1" ]; do
  case "$1" in
    -h ) 
      usage                                                                 # Mostramos la ayuda al usuario
      exit 0
      ;;
    -z ) 
      lineas_sid0=1                                                         # Activamos la opción de mostrar los procesos con SID = 0
      shift
      ;;
    -u )
      shift                                                                 # Mientras se sigan leyendo usuarios, los verifica y los guarda en un array
      while [ -n "$1" ] && [[ "$1" != -* ]]; do                             
        usuario_existe "$1"                                                 # Verificamos que el usuario existe
        usuarios+=("$1")                                                    # Guarda el usuario en el array usuarios
        shift
      done
      ;;
    -d )                                                                    
      shift                                                                 # Activamos la opción de ver los procesos de un directorio en específico
      if [ -d "$1" ]; then
        directorio="$1"                                                     # Guardamos el directorio especificado por -d
      else
        echo "Error: El directorio '$1' no existe o no es válido."          # Si el directorio no es válido, salimos con error
        exit 1
      fi
      shift
      ;;
    -t ) 
      mostrar_terminal=1                                                    # Activamos la opción -t para filtrar por terminal controladora
      shift
      ;;
    -e ) 
      lista_procesos=1                                                      # Activamos la opción -e para mostrar la tabla de procesos (funcionamiento inicial)
      shift
      ;;
    * )
      echo "parámetro '$1' no aceptado"                                     # Para cualquier otro parámetro, mostramos ayuda y salimos con error
      usage
      exit 1
      ;;
  esac
done

if [ $lista_procesos -eq 1 ]; then
  mostrar_tabla_procesos
else 
  mostrar_tabla_sesiones
fi