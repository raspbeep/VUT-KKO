#!/bin/bash

make

RED='\033[0;31m'
GREEN='\033[0;32m'
ORANGE='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

TESTSRUN=0
TESTSPASSED=0

STATS_ORIGINALSIZE=()
STATS_COMPRESSEDSIZE=()
STATS_EFFICIENCY=()
STATS_COMPRESSTIME=()

TIMEFORMAT=%R

echo "------------------------------------------"
echo -e "${BLUE}Running tests for lz_codec${NC}"
echo "------------------------------------------"

POSSIBLEFLAGS=("" "-m" "-a" "-m -a")
ALLFILES=()

for file in benchmark/*.raw
do
    # add the file to the list of all files if it is not already there
    if [[ ! " ${ALLFILES[@]} " =~ " ${file} " ]]
    then
        ALLFILES+=("$file")
    fi

    # Extract width from filename
    # filename_only will be like "800-dog.raw"
    filename_only="${file##*/}"
    # width will be like "800"
    width_from_filename="${filename_only%%-*}"

    # Check if width_from_filename is a valid number (optional, but good practice)
    if ! [[ "$width_from_filename" =~ ^[0-9]+$ ]]; then
        echo -e "${RED}Error: Could not extract a valid width from filename ${ORANGE}${filename_only}${RED}. Expected format: width-name.raw${NC}"
        echo "Skipping tests for this file."
        echo "------------------------------------------"
        continue
    fi

    for flag in "${POSSIBLEFLAGS[@]}"
    do
        rm -f compressed.tmp decompressed.tmp
        if [ -z "$flag" ]
        then
            echo "Running test for $file (width: $width_from_filename) with no flags"
        else
            echo "Running test for $file (width: $width_from_filename) with flags $flag"
        fi
        ORIGINALSIZE=$(stat -c%s "$file")

        # time ./lz_codec -c -i $file -o compressed.tmp -w 512 $flag
        # MODIFIED LINE: Use $width_from_filename instead of 512
        COMPRESSTIME=$( { time ./lz_codec -c -i "$file" -o compressed.tmp -w "$width_from_filename" $flag; } 2>&1 )
        # take only the last line of the output
        COMPRESSTIME=$(echo "$COMPRESSTIME" | tail -n 1)
        if [ $? -ne 0 ]
        then
            echo -e "${RED}Test failed on ${ORANGE}execution of compression${NC}"
            TESTSRUN=$((TESTSRUN+1))
            echo "------------------------------------------"
            continue
        fi
        COMPRESSEDSIZE=$(stat -c%s "compressed.tmp")
        # set the efficiency as the number of bits required to store one pixel (in BITS)
        # Assuming ORIGINALSIZE is total bytes for a 2D image, and each pixel takes some bytes
        # If ORIGINALSIZE is pixels * bytes_per_pixel, then ORIGINALSIZE / bytes_per_pixel would be total pixels.
        # The original script's efficiency definition seems to be bits per byte of original data if it's raw byte stream.
        # If the .raw file means raw pixels, and each pixel is 1 byte (grayscale) or 3 bytes (RGB),
        # for pixel-based efficiency, total pixels = ORIGINALSIZE / (bytes per pixel).
        # However, without knowing the pixel depth, bits per input byte is a common measure for general data compressors.
        # Let's stick to the original script's apparent definition: compressed bits / original bytes.
        EFFICIENCY=$(echo "scale=2; $COMPRESSEDSIZE*8/$ORIGINALSIZE" | bc)
        
        STATS_ORIGINALSIZE+=("$ORIGINALSIZE")
        STATS_COMPRESSEDSIZE+=("$COMPRESSEDSIZE")
        STATS_EFFICIENCY+=("$EFFICIENCY")
        STATS_COMPRESSTIME+=("$COMPRESSTIME")

        ./lz_codec -d -i compressed.tmp -o decompressed.tmp $flag
        if [ $? -ne 0 ]
        then
            echo -e "${RED}Test failed on ${ORANGE}execution of decompression${NC}"
            TESTSRUN=$((TESTSRUN+1))
            echo "------------------------------------------"
            continue
        fi
        diff "$file" decompressed.tmp > /dev/null
        if [ $? -eq 0 ]
        then
            TESTSPASSED=$((TESTSPASSED+1))
            echo -e "${GREEN}Test passed${NC}"
        else
            echo -e "${RED}Test failed on ${ORANGE}content diff${NC}"
        fi
        TESTSRUN=$((TESTSRUN+1))
        echo "------------------------------------------"
    done
done

echo "Tests run: $TESTSRUN"
echo "Tests passed: $TESTSPASSED"
echo "------------------------------------------"
echo "Compression stats:"
echo "------------------------------------------"
# print the stats in a nice table
printf "%-30s %-10s %-10s %-10s %-10s %-10s\n" "File" "Flags" "Orig. size" "Comp. size" "bpc" "Time"
# Calculate the number of flag combinations once
num_flags=${#POSSIBLEFLAGS[@]}
if [ $num_flags -eq 0 ]; then num_flags=1; fi # Avoid division by zero if POSSIBLEFLAGS is empty

for (( i=0; i<${#ALLFILES[@]}; i++ ))
do
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ ))
    do
        stat_index=$((i * num_flags + j))
        # if the flag is empty, print "No flags"
        if [ -z "${POSSIBLEFLAGS[$j]}" ]
        then
            printf "%-30s %-10s %-10s %-10s %-10s %-10s\n" "${ALLFILES[$i]##*/}" "No flags" "${STATS_ORIGINALSIZE[$stat_index]}" "${STATS_COMPRESSEDSIZE[$stat_index]}" "${STATS_EFFICIENCY[$stat_index]}" "${STATS_COMPRESSTIME[$stat_index]}"
        else
            printf "%-30s %-10s %-10s %-10s %-10s %-10s\n" "${ALLFILES[$i]##*/}" "${POSSIBLEFLAGS[$j]}" "${STATS_ORIGINALSIZE[$stat_index]}" "${STATS_COMPRESSEDSIZE[$stat_index]}" "${STATS_EFFICIENCY[$stat_index]}" "${STATS_COMPRESSTIME[$stat_index]}"
        fi
    done
done

# if the script was called with the --latex flag, print the stats in a latex table. Otherwise, just print the stats in a nice table
# the file names are printed with the path, so we need to remove the path
# print two tables, one for the efficiency and one for the compression time (in seconds).
# each table has one column for filename and then columns each for the different flags

if [ "$1" == "--latex" ]
then
    # print the start of a latex table
    echo "\begin{table}[H]"
    echo "\centering"
    # print as many columns as there are flag options plus one for the filename
    echo "\begin{tabular}{|l|*{${#POSSIBLEFLAGS[@]}}{c|}}"
    echo "\hline"
    # print the filename column
    echo -n "File & "
    # print the flag options as the column headers
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ ))
    do
        flag_display_name="${POSSIBLEFLAGS[$j]}"
        if [ -z "$flag_display_name" ]; then flag_display_name="No flags"; fi

        # Escape special LaTeX characters in flags if any, e.g., "_" or "%"
        flag_display_name="${flag_display_name//\\/\\textbackslash}" # \ -> \textbackslash
        flag_display_name="${flag_display_name//&/\\&}"            # & -> \&
        flag_display_name="${flag_display_name//%/\\%}"            # % -> \%
        flag_display_name="${flag_display_name//\$/\\\$}"          # $ -> \$
        flag_display_name="${flag_display_name//#/\\#}"           # # -> \#
        flag_display_name="${flag_display_name//_/\\_}"            # _ -> \_
        flag_display_name="${flag_display_name//\{/\\\{}"          # { -> \{
        flag_display_name="${flag_display_name//\}/\\\}}"          # } -> \}
        flag_display_name="${flag_display_name//~/\\textasciitilde}" # ~ -> \textasciitilde
        flag_display_name="${flag_display_name//^/\\textasciicircum}" # ^ -> \textasciicircum


        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]
        then
            echo -n "$flag_display_name \\\\"
        else
            echo -n "$flag_display_name & "
        fi
    done
    echo "" # Newline
    echo "\hline"

    for (( i=0; i<${#ALLFILES[@]}; i++ ))
    do
        # print the filename (escape special LaTeX chars)
        filename_display="${ALLFILES[$i]##*/}"
        filename_display="${filename_display//_/\\_}" # Escape underscores
        echo -n "$filename_display & "
        for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ ))
        do
            stat_index=$((i * num_flags + j))
            # if last flag (use the j variable), don't print the &
            if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]
            then
                echo -n "${STATS_EFFICIENCY[$stat_index]} \\\\"
            else
                echo -n "${STATS_EFFICIENCY[$stat_index]} & "
            fi
        done
        echo "" # Newline
        echo "\hline"
    done
    # print double thick line at the end of the table
    echo "\hline"
    # print the average efficiency
    echo -n "\textbf{Average} & "
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ ))
    do
        AVGPERFLAG=0
        COUNT=0
        for (( i=0; i<${#ALLFILES[@]}; i++ ))
        do
            stat_index=$((i * num_flags + j))
            # Ensure the value is not empty or non-numeric before adding
            if [[ "${STATS_EFFICIENCY[$stat_index]}" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                AVGPERFLAG=$(echo "$AVGPERFLAG + ${STATS_EFFICIENCY[$stat_index]}" | bc -l)
                COUNT=$((COUNT + 1))
            fi
        done
        if [ $COUNT -gt 0 ]; then
            AVGPERFLAG=$(echo "$AVGPERFLAG / $COUNT" | bc -l)
            AVGPERFLAG=$(printf "%.2f" $AVGPERFLAG)
        else
            AVGPERFLAG="N/A" # Or some other placeholder
        fi
        
        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]
        then
            echo -n "$AVGPERFLAG \\\\"
        else
            echo -n "$AVGPERFLAG & "
        fi
    done
    echo "" # Newline
    echo "\hline"
    # print the end of the table
    echo "\end{tabular}"
    echo "\caption{Efficiency of compression (compressed bits per original byte) for different files and flags. Lower is better.}"
    echo "\label{tab:efficiency}"
    echo "\end{table}"

    # print the start of a latex table for compression time
    echo ""
    echo "\begin{table}[H]"
    echo "\centering"
    echo "\begin{tabular}{|l|*{${#POSSIBLEFLAGS[@]}}{c|}}"
    echo "\hline"
    echo -n "File & "
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ ))
    do
        flag_display_name="${POSSIBLEFLAGS[$j]}"
        if [ -z "$flag_display_name" ]; then flag_display_name="No flags"; fi
        # Escape special LaTeX characters
        flag_display_name="${flag_display_name//\\/\\textbackslash}" 
        flag_display_name="${flag_display_name//&/\\&}"            
        flag_display_name="${flag_display_name//%/\\%}"            
        flag_display_name="${flag_display_name//\$/\\\$}"          
        flag_display_name="${flag_display_name//#/\\#}"           
        flag_display_name="${flag_display_name//_/\\_}"            
        flag_display_name="${flag_display_name//\{/\\\{}"          
        flag_display_name="${flag_display_name//\}/\\\}}"          
        flag_display_name="${flag_display_name//~/\\textasciitilde}" 
        flag_display_name="${flag_display_name//^/\\textasciicircum}" 

        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]
        then
            echo -n "$flag_display_name \\\\"
        else
            echo -n "$flag_display_name & "
        fi
    done
    echo "" # Newline
    echo "\hline"

    for (( i=0; i<${#ALLFILES[@]}; i++ ))
    do
        filename_display="${ALLFILES[$i]##*/}"
        filename_display="${filename_display//_/\\_}" # Escape underscores
        echo -n "$filename_display & "
        for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ ))
        do
            stat_index=$((i * num_flags + j))
            if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]
            then
                echo -n "${STATS_COMPRESSTIME[$stat_index]} s \\\\"
            else
                echo -n "${STATS_COMPRESSTIME[$stat_index]} s & "
            fi
        done
        echo "" # Newline
        echo "\hline"
    done
    echo "\hline"
    echo -n "\textbf{Average} & "
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ ))
    do
        AVGPERFLAG=0
        COUNT=0
        for (( i=0; i<${#ALLFILES[@]}; i++ ))
        do
            stat_index=$((i * num_flags + j))
            # Ensure the value is not empty or non-numeric before adding
            if [[ "${STATS_COMPRESSTIME[$stat_index]}" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                AVGPERFLAG=$(echo "$AVGPERFLAG + ${STATS_COMPRESSTIME[$stat_index]}" | bc -l)
                COUNT=$((COUNT + 1))
            fi
        done
         if [ $COUNT -gt 0 ]; then
            AVGPERFLAG=$(echo "$AVGPERFLAG / $COUNT" | bc -l)
            AVGPERFLAG=$(printf "%.2f" $AVGPERFLAG)
        else
            AVGPERFLAG="N/A"
        fi

        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]
        then
            echo -n "$AVGPERFLAG s\\\\"
        else
            echo -n "$AVGPERFLAG s& "
        fi
    done
    echo "" # Newline
    echo "\hline"
    echo "\end{tabular}"
    echo "\caption{Compression time (seconds) for different files and flags.}"
    echo "\label{tab:compressiontime}"
    echo "\end{table}"
fi

rm -f compressed.tmp decompressed.tmp