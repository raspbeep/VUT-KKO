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
STATS_DECOMPRESSTIME=()

TIMEFORMAT=%R

echo "------------------------------------------"
echo -e "${BLUE}Running tests for lz_codec${NC}"
echo "------------------------------------------"

POSSIBLEFLAGS=("" "-m" "-a" "-m -a")
ALLFILES_COLLECT=()

for file_path in benchmark/*.raw
do
    if [ ! -f "$file_path" ]; then
        echo -e "${ORANGE}Warning: No .raw files found in benchmark/ directory, or benchmark/ directory does not exist.${NC}"
        break
    fi
    is_present=0
    for existing_file in "${ALLFILES_COLLECT[@]}"; do
        if [[ "$existing_file" == "$file_path" ]]; then
            is_present=1
            break
        fi
    done
    if [[ "$is_present" -eq 0 ]]; then
        ALLFILES_COLLECT+=("$file_path")
    fi
done

if [ ${#ALLFILES_COLLECT[@]} -eq 0 ]; then
    echo -e "${RED}No test files found. Exiting.${NC}"
    exit 1
fi

for file in "${ALLFILES_COLLECT[@]}"
do
    filename_only="${file##*/}"
    width_from_filename="${filename_only%%-*}"

    if ! [[ "$width_from_filename" =~ ^[0-9]+$ ]]; then
        echo -e "${RED}Error: Could not extract a valid width from filename ${ORANGE}${filename_only}${RED}. Expected format: width-name.raw${NC}"
        echo "Skipping tests for this file: $file"
        # To maintain table regularity if this file was critical for stat_index,
        # one might add placeholder stats for all its flags here.
        # For simplicity, we skip, and the print loop's stat_index relies on actual entries.
        echo "------------------------------------------"
        continue
    fi

    for flag in "${POSSIBLEFLAGS[@]}"
    do
        TESTSRUN=$((TESTSRUN+1))
        rm -f compressed.tmp decompressed.tmp

        flag_desc="$flag"
        if [ -z "$flag" ]; then
            flag_desc="no flags"
        fi
        echo "Running test for $file (width: $width_from_filename) with flags: $flag_desc"

        ORIGINALSIZE=$(stat -c%s "$file")

        current_comp_size="N/A"
        current_efficiency="N/A"
        current_comp_time="Fail"
        current_decomp_time="N/A"

        COMPRESSTIME_RAW=$( { time ./lz_codec -c -i "$file" -o compressed.tmp -w "$width_from_filename" $flag; } 2>&1 )
        COMPRESS_EXIT_CODE=$?

        if [ $COMPRESS_EXIT_CODE -ne 0 ]; then
            echo -e "${RED}Test failed on ${ORANGE}execution of compression${NC}"
            echo -e "${ORANGE}lz_codec -c output:\n$COMPRESSTIME_RAW${NC}"
            current_decomp_time="N/A"
        else
            current_comp_time=$(echo "$COMPRESSTIME_RAW" | tail -n 1)
            if [ -f "compressed.tmp" ]; then
                current_comp_size=$(stat -c%s "compressed.tmp")
                if [[ "$ORIGINALSIZE" -ne 0 ]]; then
                    if [[ "$current_comp_size" =~ ^[0-9]+$ ]]; then
                         current_efficiency=$(echo "scale=2; $current_comp_size*8/$ORIGINALSIZE" | bc)
                    else
                        current_efficiency="Error"
                    fi
                else
                    current_efficiency="Inf"
                fi
            else
                echo -e "${RED}Error: compressed.tmp not created despite compression command success.${NC}"
                current_comp_size="Error"
                current_efficiency="Error"
                current_comp_time="Error" # Mark time as error too if output file missing
            fi

            current_decomp_time="Fail"
            DECOMP_OUTPUT_AND_TIME=$( { time ./lz_codec -d -i compressed.tmp -o decompressed.tmp $flag; } 2>&1 )
            DECOMP_EXIT_CODE=$?

            if [ $DECOMP_EXIT_CODE -ne 0 ]; then
                echo -e "${RED}Test failed on ${ORANGE}execution of decompression${NC}"
                echo -e "${ORANGE}lz_codec -d output:\n$DECOMP_OUTPUT_AND_TIME${NC}"
            else
                current_decomp_time=$(echo "$DECOMP_OUTPUT_AND_TIME" | tail -n 1)
                
                if [ -f "decompressed.tmp" ]; then
                    diff "$file" decompressed.tmp > /dev/null
                    DIFF_EXIT_CODE=$?
                    if [ $DIFF_EXIT_CODE -eq 0 ]; then
                        # Only count as passed if all steps (compress exec, decompress exec, diff) were OK
                        if [ $COMPRESS_EXIT_CODE -eq 0 ] && [ $DECOMP_EXIT_CODE -eq 0 ]; then
                           TESTSPASSED=$((TESTSPASSED+1))
                           echo -e "${GREEN}Test passed${NC}"
                        else
                           # This case should ideally be caught by prior error messages
                           echo -e "${RED}Test failed (content diff OK, but a prior execution step failed)${NC}"
                        fi
                    else
                        echo -e "${RED}Test failed on ${ORANGE}content diff${NC}"
                    fi
                else
                    echo -e "${RED}Test failed: decompressed.tmp not found for diff.${NC}"
                fi
            fi
        fi
        
        STATS_ORIGINALSIZE+=("$ORIGINALSIZE")
        STATS_COMPRESSEDSIZE+=("$current_comp_size")
        STATS_EFFICIENCY+=("$current_efficiency")
        STATS_COMPRESSTIME+=("$current_comp_time")
        STATS_DECOMPRESSTIME+=("$current_decomp_time")
        
        echo "------------------------------------------"
    done
done

echo "Tests run: $TESTSRUN"
echo "Tests passed: $TESTSPASSED"
echo "------------------------------------------"
echo "Compression and Decompression stats:"
echo "------------------------------------------"
printf "%-30s %-10s %-12s %-12s %-10s %-12s %-12s\n" "File" "Flags" "Orig. size" "Comp. size" "bpc" "Comp. Time" "Decomp. Time"

num_flags=${#POSSIBLEFLAGS[@]}
if [ $num_flags -eq 0 ]; then num_flags=1; fi

actual_stat_entries=${#STATS_ORIGINALSIZE[@]}

for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
        stat_index=$((i * num_flags + j))
        
        if [ "$stat_index" -ge "$actual_stat_entries" ]; then
            continue
        fi

        file_display_name="${ALLFILES_COLLECT[$i]##*/}"
        flag_display_name="${POSSIBLEFLAGS[$j]}"
        if [ -z "$flag_display_name" ]; then
            flag_display_name="No flags"
        fi
        
        printf "%-30s %-10s %-12s %-12s %-10s %-12s %-12s\n" \
            "$file_display_name" \
            "$flag_display_name" \
            "${STATS_ORIGINALSIZE[$stat_index]}" \
            "${STATS_COMPRESSEDSIZE[$stat_index]}" \
            "${STATS_EFFICIENCY[$stat_index]}" \
            "${STATS_COMPRESSTIME[$stat_index]}" \
            "${STATS_DECOMPRESSTIME[$stat_index]}"
    done
done


if [ "$1" == "--latex" ]; then
    echo ""
    echo -e "${BLUE}Generating LaTeX tables...${NC}"
    echo ""
    
    # --- Efficiency Table ---
    echo "\begin{table}[H]"
    echo "\centering"
    echo "\begin{tabular}{|l|*{${#POSSIBLEFLAGS[@]}}{c|}}"
    echo "\hline"
    echo -n "File & "
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
        flag_display_name="${POSSIBLEFLAGS[$j]}"
        if [ -z "$flag_display_name" ]; then flag_display_name="No flags"; fi
        flag_display_name="${flag_display_name//_/\\_}" 
        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$flag_display_name \\\\"; else echo -n "$flag_display_name & "; fi
    done
    echo ""; echo "\hline"
    for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
        filename_display="${ALLFILES_COLLECT[$i]##*/}"
        filename_display="${filename_display//_/\\_}"
        echo -n "$filename_display & "
        for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
            stat_index=$((i * num_flags + j))
            value_to_print="N/A"
            if [ "$stat_index" -lt "$actual_stat_entries" ]; then value_to_print="${STATS_EFFICIENCY[$stat_index]}"; fi
            if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$value_to_print \\\\"; else echo -n "$value_to_print & "; fi
        done
        echo ""; echo "\hline"
    done
    echo "\hline"
    echo -n "\textbf{Average} & "
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
        AVGPERFLAG=0.0; COUNT=0
        for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
            stat_index=$((i * num_flags + j))
            if [ "$stat_index" -lt "$actual_stat_entries" ] && [[ "${STATS_EFFICIENCY[$stat_index]}" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                AVGPERFLAG=$(echo "$AVGPERFLAG + ${STATS_EFFICIENCY[$stat_index]}" | bc -l)
                COUNT=$((COUNT + 1))
            fi
        done
        if [ $COUNT -gt 0 ]; then AVGPERFLAG=$(printf "%.2f" $(echo "$AVGPERFLAG / $COUNT" | bc -l)); else AVGPERFLAG="N/A"; fi
        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$AVGPERFLAG \\\\"; else echo -n "$AVGPERFLAG & "; fi
    done
    echo ""; echo "\hline"
    echo "\end{tabular}"
    echo "\caption{Efficiency of compression (compressed bits per original byte). Lower is better.}"
    echo "\label{tab:efficiency}"
    echo "\end{table}"
    echo ""

    # --- Compression Time Table ---
    echo "\begin{table}[H]"
    echo "\centering"
    echo "\begin{tabular}{|l|*{${#POSSIBLEFLAGS[@]}}{c|}}"
    echo "\hline"
    echo -n "File (seconds) & "
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
        flag_display_name="${POSSIBLEFLAGS[$j]}"
        if [ -z "$flag_display_name" ]; then flag_display_name="No flags"; fi
        flag_display_name="${flag_display_name//_/\\_}"
        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$flag_display_name \\\\"; else echo -n "$flag_display_name & "; fi
    done
    echo ""; echo "\hline"
    for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
        filename_display="${ALLFILES_COLLECT[$i]##*/}"
        filename_display="${filename_display//_/\\_}"
        echo -n "$filename_display & "
        for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
            stat_index=$((i * num_flags + j))
            value_to_print="N/A"
            if [ "$stat_index" -lt "$actual_stat_entries" ]; then 
                comp_time_val="${STATS_COMPRESSTIME[$stat_index]}"
                if [[ "$comp_time_val" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                    value_to_print="$comp_time_val s"
                else
                    value_to_print="$comp_time_val" 
                fi
            fi
            if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$value_to_print \\\\"; else echo -n "$value_to_print & "; fi
        done
        echo ""; echo "\hline"
    done
    echo "\hline"
    echo -n "\textbf{Average} & "
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
        AVGPERFLAG=0.0; COUNT=0
        for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
            stat_index=$((i * num_flags + j))
            if [ "$stat_index" -lt "$actual_stat_entries" ] && [[ "${STATS_COMPRESSTIME[$stat_index]}" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                AVGPERFLAG=$(echo "$AVGPERFLAG + ${STATS_COMPRESSTIME[$stat_index]}" | bc -l)
                COUNT=$((COUNT + 1))
            fi
        done
        if [ $COUNT -gt 0 ]; then AVGPERFLAG=$(printf "%.3f" $(echo "$AVGPERFLAG / $COUNT" | bc -l)); else AVGPERFLAG="N/A"; fi # Times with .3f
        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$AVGPERFLAG s\\\\"; else echo -n "$AVGPERFLAG s& "; fi
    done
    echo ""; echo "\hline"
    echo "\end{tabular}"
    echo "\caption{Compression time (seconds) for different files and flags.}"
    echo "\label{tab:compressiontime}"
    echo "\end{table}"
    echo ""

    # --- Decompression Time Table ---
    echo "\begin{table}[H]"
    echo "\centering"
    echo "\begin{tabular}{|l|*{${#POSSIBLEFLAGS[@]}}{c|}}"
    echo "\hline"
    echo -n "File (seconds) & " 
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
        flag_display_name="${POSSIBLEFLAGS[$j]}"
        if [ -z "$flag_display_name" ]; then flag_display_name="No flags"; fi
        flag_display_name="${flag_display_name//_/\\_}"
        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$flag_display_name \\\\"; else echo -n "$flag_display_name & "; fi
    done
    echo ""; echo "\hline"
    for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
        filename_display="${ALLFILES_COLLECT[$i]##*/}"
        filename_display="${filename_display//_/\\_}"
        echo -n "$filename_display & "
        for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
            stat_index=$((i * num_flags + j))
            value_to_print="N/A"
            if [ "$stat_index" -lt "$actual_stat_entries" ]; then
                decomp_time_val="${STATS_DECOMPRESSTIME[$stat_index]}"
                if [[ "$decomp_time_val" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                    value_to_print="$decomp_time_val s"
                else
                    value_to_print="$decomp_time_val"
                fi
            fi
            if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$value_to_print \\\\"; else echo -n "$value_to_print & "; fi
        done
        echo ""; echo "\hline"
    done
    echo "\hline"
    echo -n "\textbf{Average} & "
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
        AVGPERFLAG=0.0; COUNT=0
        for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
            stat_index=$((i * num_flags + j))
            if [ "$stat_index" -lt "$actual_stat_entries" ] && [[ "${STATS_DECOMPRESSTIME[$stat_index]}" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                AVGPERFLAG=$(echo "$AVGPERFLAG + ${STATS_DECOMPRESSTIME[$stat_index]}" | bc -l)
                COUNT=$((COUNT + 1))
            fi
        done
        if [ $COUNT -gt 0 ]; then AVGPERFLAG=$(printf "%.3f" $(echo "$AVGPERFLAG / $COUNT" | bc -l)); else AVGPERFLAG="N/A"; fi # Times with .3f
        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$AVGPERFLAG s\\\\"; else echo -n "$AVGPERFLAG s& "; fi
    done
    echo ""; echo "\hline"
    echo "\end{tabular}"
    echo "\caption{Decompression time (seconds) for different files and flags.}"
    echo "\label{tab:decompressiontime}"
    echo "\end{table}"
fi

# --- Overall Averages Calculation and Print ---
total_comp_time_sum=0.0
valid_comp_time_count=0
total_decomp_time_sum=0.0
valid_decomp_time_count=0
total_bpc_sum=0.0
valid_bpc_count=0

# Use actual_stat_entries which is the total number of records in STATS arrays
for ((k=0; k<actual_stat_entries; k++)); do
    comp_time_val="${STATS_COMPRESSTIME[$k]}"
    if [[ "$comp_time_val" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        total_comp_time_sum=$(echo "$total_comp_time_sum + $comp_time_val" | bc -l)
        valid_comp_time_count=$((valid_comp_time_count + 1))
    fi

    decomp_time_val="${STATS_DECOMPRESSTIME[$k]}"
    if [[ "$decomp_time_val" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        total_decomp_time_sum=$(echo "$total_decomp_time_sum + $decomp_time_val" | bc -l)
        valid_decomp_time_count=$((valid_decomp_time_count + 1))
    fi

    bpc_val="${STATS_EFFICIENCY[$k]}"
    if [[ "$bpc_val" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        total_bpc_sum=$(echo "$total_bpc_sum + $bpc_val" | bc -l)
        valid_bpc_count=$((valid_bpc_count + 1))
    fi
done

avg_comp_time_overall="N/A"
if [ "$valid_comp_time_count" -gt 0 ]; then
    avg_comp_time_overall=$(printf "%.3f" $(echo "$total_comp_time_sum / $valid_comp_time_count" | bc -l))
fi

avg_decomp_time_overall="N/A"
if [ "$valid_decomp_time_count" -gt 0 ]; then
    avg_decomp_time_overall=$(printf "%.3f" $(echo "$total_decomp_time_sum / $valid_decomp_time_count" | bc -l))
fi

avg_bpc_overall="N/A"
if [ "$valid_bpc_count" -gt 0 ]; then
    avg_bpc_overall=$(printf "%.2f" $(echo "$total_bpc_sum / $valid_bpc_count" | bc -l))
fi

echo ""
echo "------------------------------------------"
echo -e "${BLUE}Overall Averages Across All Configurations:${NC}"
echo "Average Compression Time: $avg_comp_time_overall s"
echo "Average Decompression Time: $avg_decomp_time_overall s"
echo "Average bpc (bits per original byte): $avg_bpc_overall"
echo "------------------------------------------"
# End Overall Averages

rm -f compressed.tmp decompressed.tmp