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
STATS_DECOMPRESSTIME=() # New array for decompression times

TIMEFORMAT=%R

echo "------------------------------------------"
echo -e "${BLUE}Running tests for lz_codec${NC}"
echo "------------------------------------------"

POSSIBLEFLAGS=("" "-m" "-a" "-m -a")
ALLFILES_COLLECT=() # Use a different name for collection to avoid confusion if script is re-run in same shell

# First, collect all unique .raw files from benchmark directory
for file_path in benchmark/*.raw
do
    # Check if file exists to prevent errors if no .raw files are present
    if [ ! -f "$file_path" ]; then
        echo -e "${ORANGE}Warning: No .raw files found in benchmark/ directory, or benchmark/ directory does not exist.${NC}"
        break
    fi
    # add the file to the list of all files if it is not already there
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


for file in "${ALLFILES_COLLECT[@]}" # Iterate over unique collected files
do
    filename_only="${file##*/}"
    width_from_filename="${filename_only%%-*}"

    if ! [[ "$width_from_filename" =~ ^[0-9]+$ ]]; then
        echo -e "${RED}Error: Could not extract a valid width from filename ${ORANGE}${filename_only}${RED}. Expected format: width-name.raw${NC}"
        echo "Skipping tests for this file: $file"
        # Add placeholders for all flags for this skipped file to keep table structure if absolutely needed
        # For simplicity here, we'll just skip and table might look irregular or adapt printing.
        # The current STATS addition strategy relies on processing each file an flag combo.
        # To maintain table regularity for printing loops later, we'd add placeholders here.
        # However, the problem is more with the `stat_index` calculation in print if some `ALLFILES_COLLECT` are skipped.
        # For now, the original script did not add placeholders for skipped files, so we continue this.
        # The final print loop uses `ALLFILES_COLLECT` length, so if a file is fully skipped, that row won't appear.
        # This is acceptable.
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

        # Initialize current stats for this iteration
        current_comp_size="N/A"
        current_efficiency="N/A"
        current_comp_time="Fail"
        current_decomp_time="N/A" # Will be 'Fail' if compress fails or decomp fails

        # Compression
        COMPRESSTIME_RAW=$( { time ./lz_codec -c -i "$file" -o compressed.tmp -w "$width_from_filename" $flag; } 2>&1 )
        COMPRESS_EXIT_CODE=$?

        if [ $COMPRESS_EXIT_CODE -ne 0 ]; then
            echo -e "${RED}Test failed on ${ORANGE}execution of compression${NC}"
            echo -e "${ORANGE}lz_codec -c output:\n$COMPRESSTIME_RAW${NC}"
            current_decomp_time="N/A" # Compression failed, so no decompression attempt
        else
            current_comp_time=$(echo "$COMPRESSTIME_RAW" | tail -n 1)
            if [ -f "compressed.tmp" ]; then
                current_comp_size=$(stat -c%s "compressed.tmp")
                if [[ "$ORIGINALSIZE" -ne 0 ]]; then # Avoid division by zero
                    if [[ "$current_comp_size" =~ ^[0-9]+$ ]]; then # Ensure comp_size is a number
                         current_efficiency=$(echo "scale=2; $current_comp_size*8/$ORIGINALSIZE" | bc)
                    else
                        current_efficiency="Error" # Should not happen if stat worked
                    fi
                else # Original size is 0
                    current_efficiency="Inf" # Or "N/A" or "0.00" depending on preference
                fi
            else
                echo -e "${RED}Error: compressed.tmp not created despite compression command success.${NC}"
                current_comp_size="Error"
                current_efficiency="Error"
            fi

            # Decompression
            current_decomp_time="Fail" # Default if decompression fails
            DECOMP_OUTPUT_AND_TIME=$( { time ./lz_codec -d -i compressed.tmp -o decompressed.tmp $flag; } 2>&1 )
            DECOMP_EXIT_CODE=$?

            if [ $DECOMP_EXIT_CODE -ne 0 ]; then
                echo -e "${RED}Test failed on ${ORANGE}execution of decompression${NC}"
                echo -e "${ORANGE}lz_codec -d output:\n$DECOMP_OUTPUT_AND_TIME${NC}"
            else
                current_decomp_time=$(echo "$DECOMP_OUTPUT_AND_TIME" | tail -n 1)
                
                # Diff check (only if compression and decompression commands were successful)
                if [ -f "decompressed.tmp" ]; then
                    diff "$file" decompressed.tmp > /dev/null
                    DIFF_EXIT_CODE=$?
                    if [ $DIFF_EXIT_CODE -eq 0 ]; then
                        TESTSPASSED=$((TESTSPASSED+1))
                        echo -e "${GREEN}Test passed${NC}"
                    else
                        echo -e "${RED}Test failed on ${ORANGE}content diff${NC}"
                    fi
                else
                    echo -e "${RED}Test failed: decompressed.tmp not found for diff.${NC}"
                fi
            fi # End decompression execution check
        fi # End compression execution check
        
        # Add stats for this iteration
        STATS_ORIGINALSIZE+=("$ORIGINALSIZE")
        STATS_COMPRESSEDSIZE+=("$current_comp_size")
        STATS_EFFICIENCY+=("$current_efficiency")
        STATS_COMPRESSTIME+=("$current_comp_time")
        STATS_DECOMPRESSTIME+=("$current_decomp_time")
        
        echo "------------------------------------------"
    done # End flag loop
done # End file loop

echo "Tests run: $TESTSRUN"
echo "Tests passed: $TESTSPASSED"
echo "------------------------------------------"
echo "Compression and Decompression stats:"
echo "------------------------------------------"
printf "%-30s %-10s %-12s %-12s %-10s %-12s %-12s\n" "File" "Flags" "Orig. size" "Comp. size" "bpc" "Comp. Time" "Decomp. Time"

num_flags=${#POSSIBLEFLAGS[@]}
if [ $num_flags -eq 0 ]; then num_flags=1; fi

actual_stat_entries=${#STATS_ORIGINALSIZE[@]} # Number of actual entries collected

# The number of rows should be actual_stat_entries / num_flags if all files completed all flags
# Or, more simply, iterate based on ALLFILES_COLLECT and POSSIBLEFLAGS, using the stat_index
for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
        stat_index=$((i * num_flags + j))
        
        # Check if stat_index is within bounds of collected stats
        if [ "$stat_index" -ge "$actual_stat_entries" ]; then
            # This case should ideally not be hit if placeholders are added for all file/flag combos processed
            # Or if a file was fully skipped due to width extraction error, this combo won't be processed.
            # For robustness, we can check.
            # echo "Warning: stat_index $stat_index out of bounds ($actual_stat_entries)"
            continue # Or print N/A fields
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
        flag_display_name="${flag_display_name//_/\\_}" # Basic LaTeX escape
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
        AVGPERFLAG=0; COUNT=0
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
    echo -n "File (seconds) & " # Clarified unit in header
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
            if [ "$stat_index" -lt "$actual_stat_entries" ]; then value_to_print="${STATS_COMPRESSTIME[$stat_index]} s"; fi
            if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$value_to_print \\\\"; else echo -n "$value_to_print & "; fi
        done
        echo ""; echo "\hline"
    done
    echo "\hline"
    echo -n "\textbf{Average} & "
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
        AVGPERFLAG=0; COUNT=0
        for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
            stat_index=$((i * num_flags + j))
            if [ "$stat_index" -lt "$actual_stat_entries" ] && [[ "${STATS_COMPRESSTIME[$stat_index]}" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                AVGPERFLAG=$(echo "$AVGPERFLAG + ${STATS_COMPRESSTIME[$stat_index]}" | bc -l)
                COUNT=$((COUNT + 1))
            fi
        done
        if [ $COUNT -gt 0 ]; then AVGPERFLAG=$(printf "%.2f" $(echo "$AVGPERFLAG / $COUNT" | bc -l)); else AVGPERFLAG="N/A"; fi
        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$AVGPERFLAG s\\\\"; else echo -n "$AVGPERFLAG s& "; fi
    done
    echo ""; echo "\hline"
    echo "\end{tabular}"
    echo "\caption{Compression time (seconds) for different files and flags.}"
    echo "\label{tab:compressiontime}"
    echo "\end{table}"
    echo ""

    # --- Decompression Time Table (NEW) ---
    echo "\begin{table}[H]"
    echo "\centering"
    echo "\begin{tabular}{|l|*{${#POSSIBLEFLAGS[@]}}{c|}}"
    echo "\hline"
    echo -n "File (seconds) & " # Clarified unit in header
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
            # Ensure value exists and add "s" for seconds, handle "Fail" etc.
            if [ "$stat_index" -lt "$actual_stat_entries" ]; then
                decomp_time_val="${STATS_DECOMPRESSTIME[$stat_index]}"
                if [[ "$decomp_time_val" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                    value_to_print="$decomp_time_val s"
                else
                    value_to_print="$decomp_time_val" # Print "Fail", "N/A" as is
                fi
            fi
            if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$value_to_print \\\\"; else echo -n "$value_to_print & "; fi
        done
        echo ""; echo "\hline"
    done
    echo "\hline"
    echo -n "\textbf{Average} & "
    for (( j=0; j<${#POSSIBLEFLAGS[@]}; j++ )); do
        AVGPERFLAG=0; COUNT=0
        for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
            stat_index=$((i * num_flags + j))
            if [ "$stat_index" -lt "$actual_stat_entries" ] && [[ "${STATS_DECOMPRESSTIME[$stat_index]}" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                AVGPERFLAG=$(echo "$AVGPERFLAG + ${STATS_DECOMPRESSTIME[$stat_index]}" | bc -l)
                COUNT=$((COUNT + 1))
            fi
        done
        if [ $COUNT -gt 0 ]; then AVGPERFLAG=$(printf "%.2f" $(echo "$AVGPERFLAG / $COUNT" | bc -l)); else AVGPERFLAG="N/A"; fi
        if [ "$j" -eq "$((${#POSSIBLEFLAGS[@]}-1))" ]; then echo -n "$AVGPERFLAG s\\\\"; else echo -n "$AVGPERFLAG s& "; fi
    done
    echo ""; echo "\hline"
    echo "\end{tabular}"
    echo "\caption{Decompression time (seconds) for different files and flags.}"
    echo "\label{tab:decompressiontime}"
    echo "\end{table}"
fi

rm -f compressed.tmp decompressed.tmp