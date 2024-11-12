#!/bin/bash

ordenamiento=user                                 # Por defecto, la tabla se ordena por nombre de usuario
lineas_sid0=0                                     # Si no se utiliza la opción -z, se excluyen los procesos con SID = 0
usuarios=()                                       # Array de usuarios
directorio=""                                     # Directorio del que se mirarán los procesos que abran archivos en él 
mostrar_terminal=0                                # Se activa al usar -t, para mostrar procesos con terminal controladora asociada

lista_procesos=0                                  # Se activa al usar -e, para mostrar la tabla de procesos (funcionamiento principal)
ordenar_por_memoria=0                             # -sm activa esta variable
ordenar_por_grupos=0                              # -sg activa esta variable
ordenar_reversa=0                                 # -r activa esta variable

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

mostrar_tabla_sesiones() {
  # Construcción de comando `ps` con filtros activos
  ps_command="ps -eo sid,pgid,pid,user,tty,%mem,cmd --sort=sid | tail -n+2"

  # Aplicación de filtros en `ps`
  if [ ${#usuarios[@]} -gt 0 ]; then
    local users_pattern=$(IFS=\| ; echo "${usuarios[*]}")
    ps_command="$ps_command | awk -v users=\"$users_pattern\" '\$4 ~ users'"
  fi

  if [ $lineas_sid0 -eq 0 ]; then
    ps_command="$ps_command | awk '\$1 != 0'"
  fi

  if [ -n "$directorio" ]; then
    local pid_lsof=$(lsof +d "$directorio" 2>/dev/null | awk 'NR>1 {print $2}' | sort -u)
    if [ -n "$pid_lsof" ]; then
      ps_command="$ps_command | awk '{if (index(\"$pid_lsof\", \$3)) print}'"
    fi
  fi

  if [ $mostrar_terminal -eq 1 ]; then
    ps_command="$ps_command | awk '\$5 != \"?\"'"
  fi

  # Ejecutar `ps` y almacenar resultados
  procesos=$(eval $ps_command)

  # Procesar por sesión con cálculos de campos requeridos
  echo "$procesos" | awk -v sm="$ordenar_por_memoria" -v sg="$ordenar_por_grupos" -v r="$ordenar_reversa" '
  BEGIN {
    FS=" ";
    OFS="\t";
  }
  {
    sid=$1; pgid=$2; pid=$3; user=$4; tty=$5; mem=$6; cmd=$7;
    
    # Información de la sesión
    if (!(sid in total_mem)) {
      total_mem[sid] = 0;
      total_grupos[sid] = 0;
      leader_pid[sid] = "?";
      leader_user[sid] = "?";
      leader_tty[sid] = "?";
      leader_cmd[sid] = "?";
    }

    # Suma de %MEM
    total_mem[sid] += mem;

    # Contar grupos de procesos (PGID únicos)
    if (!(pgid in groups[sid])) {
      groups[sid][pgid] = 1;
      total_grupos[sid]++;
    }

    # Verificar proceso líder
    if (pid == sid) {
      leader_pid[sid] = pid;
      leader_user[sid] = user;
      leader_tty[sid] = (tty == "?") ? "?" : tty;
      leader_cmd[sid] = cmd;
    }
  }
  END {
    for (sid in total_mem) {
      # Imprimir una sola fila por sesión
      print sid, total_grupos[sid], total_mem[sid], leader_pid[sid], leader_user[sid], leader_tty[sid], leader_cmd[sid];
    }
  }' | sort -t$'\t' -k$([ $ordenar_por_grupos -eq 1 ] && echo "2" || echo "4")$([ $ordenar_por_memoria -eq 2 ] && echo "3") $([ $ordenar_reversa -eq 1 ] && echo "-r")
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
    -r )
      ordenar_reversa=1
      shift
      ;;
    -sm )
      if [ $lista_procesos -eq 1 ]; then
        ordenar_por_memoria=1
      else
        ordenar_por_memoria=2  # Usado solo en modo sesión
      fi
      shift
      ;;
    -sg )
      if [ $lista_procesos -eq 1 ] || [ $ordenar_por_memoria -eq 1 ]; then
        echo "Error: -sg no es compatible con -e o -sm"
        exit 1
      fi
      ordenar_por_grupos=1
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