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
STATS_EFFICIENCY=() # This will store bpp (bits per original byte)
STATS_COMPRESSTIME=()
STATS_DECOMPRESSTIME=()

TIMEFORMAT=%R

echo "------------------------------------------"
echo -e "${BLUE}Running tests for lz_codec${NC}"
echo "------------------------------------------"

# Order: Baseline, Model, Adaptive, Adaptive+Model
# This order is important for how data is stored and retrieved later for LaTeX tables.
POSSIBLEFLAGS=("" "-m" "-a" "-a -m") # Note: changed "-m -a" to "-a -m" to match example table text, functionally same for lz_codec if order doesn't matter
FLAG_NAMES=("Baseline" "Model (-m)" "Adaptive (-a)" "Adaptive+Model (-a -m)") # For display

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
    # Attempt to extract width based on "width-name.raw" or handle cases where it's just "name.raw"
    # This part might need adjustment based on actual naming conventions of .raw files if not strictly "width-..."
    width_from_filename_part="${filename_only%%-*}"
    
    # Basic check if the extracted part is numeric; if not, it might be a filename without explicit width prefix.
    # For this script, we assume lz_codec can handle it or width is passed differently.
    # The original script had a more complex width extraction; this is simplified.
    # If width is crucial and always in filename like "1024-...", the original check was better.
    # For now, let's assume a default or that lz_codec handles it if not found.
    # This script will use a placeholder if not extractable, or you might need to adjust lz_codec call.
    width_param=""
    if [[ "$width_from_filename_part" =~ ^[0-9]+$ ]]; then
        width_param="-w $width_from_filename_part"
        echo "Detected width: $width_from_filename_part for $filename_only"
    else
        echo -e "${ORANGE}Warning: Could not extract numeric width from start of filename ${filename_only}. Proceeding without explicit -w flag unless lz_codec requires it.${NC}"
        # If lz_codec absolutely needs a width, this branch should error out or use a default.
    fi


    for idx in "${!POSSIBLEFLAGS[@]}"
    do
        flag="${POSSIBLEFLAGS[$idx]}"
        TESTSRUN=$((TESTSRUN+1))
        rm -f compressed.tmp decompressed.tmp

        flag_desc="${FLAG_NAMES[$idx]}"
        echo "Running test for $file ${width_param} with flags: $flag_desc"

        ORIGINALSIZE=$(stat -c%s "$file")

        current_comp_size="N/A"
        current_bpp="N/A" # Changed from efficiency to bpp
        current_comp_time="Fail"
        current_decomp_time="N/A"

        # Execute compression
        # shellcheck disable=SC2086
        COMPRESSTIME_RAW=$( { time ./lz_codec -c -i "$file" -o compressed.tmp $width_param $flag; } 2>&1 )
        COMPRESS_EXIT_CODE=$?


        if [ $COMPRESS_EXIT_CODE -ne 0 ]; then
            echo -e "${RED}Test failed on ${ORANGE}execution of compression${NC}"
            echo -e "${ORANGE}lz_codec -c output:\n$COMPRESSTIME_RAW${NC}"
            current_decomp_time="N/A" # Cannot decompress if compression failed
        else
            current_comp_time=$(echo "$COMPRESSTIME_RAW" | tail -n 1)
            if [ -f "compressed.tmp" ]; then
                current_comp_size=$(stat -c%s "compressed.tmp")
                if [[ "$ORIGINALSIZE" -ne 0 && "$current_comp_size" =~ ^[0-9]+$ ]]; then
                    # bpp: (compressed_size_in_bits / original_size_in_bytes) / components_per_pixel
                    # Assuming 1 component per pixel for .raw, or this needs adjustment
                    # The example tables use 'bpp' but calculate it as bits per original byte.
                    # We will follow that: (compressed_size_bytes * 8) / original_size_bytes
                    current_bpp=$(echo "scale=2; $current_comp_size * 8 / $ORIGINALSIZE" | bc)
                elif [[ "$ORIGINALSIZE" -eq 0 ]]; then
                    current_bpp="Inf"
                else
                    current_bpp="Error"
                fi
            else
                echo -e "${RED}Error: compressed.tmp not created despite compression command success.${NC}"
                current_comp_size="Error"
                current_bpp="Error"
                current_comp_time="Error"
            fi

            # Execute decompression
            current_decomp_time="Fail"
            # shellcheck disable=SC2086
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
                    if [ $DIFF_EXIT_CODE -eq 0 ];then
                        if [ $COMPRESS_EXIT_CODE -eq 0 ] && [ $DECOMP_EXIT_CODE -eq 0 ]; then
                           TESTSPASSED=$((TESTSPASSED+1))
                           echo -e "${GREEN}Test passed${NC}"
                        else
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
        STATS_EFFICIENCY+=("$current_bpp") # Storing bpp here
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
printf "%-20s %-25s %-12s %-12s %-10s %-12s %-12s\n" "File" "Flags" "Orig. size" "Comp. size" "bpp" "Comp. Time" "Decomp. Time"

num_flags=${#POSSIBLEFLAGS[@]}
actual_stat_entries=${#STATS_ORIGINALSIZE[@]}

for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
    for (( j=0; j<num_flags; j++ )); do
        stat_index=$((i * num_flags + j))
        
        if [ "$stat_index" -ge "$actual_stat_entries" ]; then
            # This should ideally not happen if loops are correct and data is populated for all file/flag combos
            echo -e "${ORANGE}Warning: Missing data for stat_index $stat_index. Skipping print.${NC}"
            continue
        fi

        file_display_name="${ALLFILES_COLLECT[$i]##*/}"
        # Ensure flag_display_name corresponds to the current j index
        flag_display_name="${FLAG_NAMES[$j]}"
        
        # Format time values if they are numeric, otherwise print as is (e.g., "Fail", "N/A")
        comp_time_val="${STATS_COMPRESSTIME[$stat_index]}"
        [[ "$comp_time_val" =~ ^[0-9]+(\.[0-9]+)?$ ]] && comp_time_print="${comp_time_val}s" || comp_time_print="$comp_time_val"
        
        decomp_time_val="${STATS_DECOMPRESSTIME[$stat_index]}"
        [[ "$decomp_time_val" =~ ^[0-9]+(\.[0-9]+)?$ ]] && decomp_time_print="${decomp_time_val}s" || decomp_time_print="$decomp_time_val"

        # Format bpp to 2 decimal places if numeric
        bpp_val="${STATS_EFFICIENCY[$stat_index]}"
        if [[ "$bpp_val" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
            bpp_print=$(printf "%.2f" "$bpp_val")
        else
            bpp_print="$bpp_val"
        fi

        printf "%-20s %-25s %-12s %-12s %-10s %-12s %-12s\n" \
            "$file_display_name" \
            "$flag_display_name" \
            "${STATS_ORIGINALSIZE[$stat_index]}" \
            "${STATS_COMPRESSEDSIZE[$stat_index]}" \
            "$bpp_print" \
            "$comp_time_print" \
            "$decomp_time_print"
    done
done


if [ "$1" == "--latex" ]; then
    echo ""
    echo -e "${BLUE}Generating LaTeX tables...${NC}"
    echo ""

    # Data indices for POSSIBLEFLAGS and FLAG_NAMES:
    # 0: Baseline ("")
    # 1: Model ("-m")
    # 2: Adaptive ("-a")
    # 3: Adaptive+Model ("-a -m")

    # --- Table 1: Baseline vs. Adaptive ---
    echo "% --- Table 1: Baseline vs. Adaptive ---"
    echo "\\begin{table}[h!]"
    echo "\\centering"
    echo "\\caption{Benchmark Results: Baseline vs. Adaptive (-a)}"
    echo "\\label{tab:baseline_adaptive_script}"
    echo "\\begin{tabular}{lrrrrrr}"
    echo "\\toprule"
    echo "          & \\multicolumn{3}{c}{Baseline} & \\multicolumn{3}{c}{Adaptive (-a)} \\\\"
    echo "\\cmidrule(lr){2-4} \\cmidrule(lr){5-7}"
    echo "File        & Comp. (s) & Decomp (s) & bpp  & Comp. (s) & Decomp (s) & bpp  \\\\"
    echo "\\midrule"
    for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
        filename_display="${ALLFILES_COLLECT[$i]##*/}"
        filename_display="${filename_display//_/\\_}" # Escape underscores for LaTeX
        
        # Baseline data (j=0)
        idx_baseline=$((i * num_flags + 0))
        ct_b="${STATS_COMPRESSTIME[$idx_baseline]}"
        dt_b="${STATS_DECOMPRESSTIME[$idx_baseline]}"
        bpp_b="${STATS_EFFICIENCY[$idx_baseline]}"
        [[ "$ct_b" =~ ^[0-9]+(\.[0-9]+)?$ ]] && ct_b_f=$(printf "%.3fs" "$ct_b") || ct_b_f="$ct_b"
        [[ "$dt_b" =~ ^[0-9]+(\.[0-9]+)?$ ]] && dt_b_f=$(printf "%.3fs" "$dt_b") || dt_b_f="$dt_b"
        [[ "$bpp_b" =~ ^[0-9]+(\.[0-9]+)?$ ]] && bpp_b_f=$(printf "%.2f" "$bpp_b") || bpp_b_f="$bpp_b"

        # Adaptive data (j=2)
        idx_adaptive=$((i * num_flags + 2))
        ct_a="${STATS_COMPRESSTIME[$idx_adaptive]}"
        dt_a="${STATS_DECOMPRESSTIME[$idx_adaptive]}"
        bpp_a="${STATS_EFFICIENCY[$idx_adaptive]}"
        [[ "$ct_a" =~ ^[0-9]+(\.[0-9]+)?$ ]] && ct_a_f=$(printf "%.3fs" "$ct_a") || ct_a_f="$ct_a"
        [[ "$dt_a" =~ ^[0-9]+(\.[0-9]+)?$ ]] && dt_a_f=$(printf "%.3fs" "$dt_a") || dt_a_f="$dt_a"
        [[ "$bpp_a" =~ ^[0-9]+(\.[0-9]+)?$ ]] && bpp_a_f=$(printf "%.2f" "$bpp_a") || bpp_a_f="$bpp_a"

        printf "%-10s & %7s & %10s & %4s & %7s & %10s & %4s \\\\\n" \
            "$filename_display" \
            "$ct_b_f" "$dt_b_f" "$bpp_b_f" \
            "$ct_a_f" "$dt_a_f" "$bpp_a_f"
    done
    echo "\\bottomrule"
    echo "\\end{tabular}"
    echo "\\end{table}"
    echo ""

    # --- Table 2: Model vs. Adaptive+Model ---
    echo "% --- Table 2: Model vs. Adaptive+Model ---"
    echo "\\begin{table}[h!]"
    echo "\\centering"
    echo "\\caption{Benchmark Results: Model (-m) vs. Adaptive\\,+\\,Model (-a -m)}" # Added \, for spacing in LaTeX
    echo "\\label{tab:model_adaptive_model_script}"
    echo "\\begin{tabular}{lrrrrrr}"
    echo "\\toprule"
    echo "          & \\multicolumn{3}{c}{Model (-m)} & \\multicolumn{3}{c}{Adaptive\\,+\\,Model (-a -m)} \\\\"
    echo "\\cmidrule(lr){2-4} \\cmidrule(lr){5-7}"
    echo "File        & Comp. (s) & Decomp (s) & bpp  & Comp. (s) & Decomp (s) & bpp  \\\\"
    echo "\\midrule"
    for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
        filename_display="${ALLFILES_COLLECT[$i]##*/}"
        filename_display="${filename_display//_/\\_}"

        # Model data (j=1)
        idx_model=$((i * num_flags + 1))
        ct_m="${STATS_COMPRESSTIME[$idx_model]}"
        dt_m="${STATS_DECOMPRESSTIME[$idx_model]}"
        bpp_m="${STATS_EFFICIENCY[$idx_model]}"
        [[ "$ct_m" =~ ^[0-9]+(\.[0-9]+)?$ ]] && ct_m_f=$(printf "%.3fs" "$ct_m") || ct_m_f="$ct_m"
        [[ "$dt_m" =~ ^[0-9]+(\.[0-9]+)?$ ]] && dt_m_f=$(printf "%.3fs" "$dt_m") || dt_m_f="$dt_m"
        [[ "$bpp_m" =~ ^[0-9]+(\.[0-9]+)?$ ]] && bpp_m_f=$(printf "%.2f" "$bpp_m") || bpp_m_f="$bpp_m"

        # Adaptive+Model data (j=3)
        idx_am=$((i * num_flags + 3))
        ct_am="${STATS_COMPRESSTIME[$idx_am]}"
        dt_am="${STATS_DECOMPRESSTIME[$idx_am]}"
        bpp_am="${STATS_EFFICIENCY[$idx_am]}"
        [[ "$ct_am" =~ ^[0-9]+(\.[0-9]+)?$ ]] && ct_am_f=$(printf "%.3fs" "$ct_am") || ct_am_f="$ct_am"
        [[ "$dt_am" =~ ^[0-9]+(\.[0-9]+)?$ ]] && dt_am_f=$(printf "%.3fs" "$dt_am") || dt_am_f="$dt_am"
        [[ "$bpp_am" =~ ^[0-9]+(\.[0-9]+)?$ ]] && bpp_am_f=$(printf "%.2f" "$bpp_am") || bpp_am_f="$bpp_am"

        printf "%-10s & %7s & %10s & %4s & %7s & %10s & %4s \\\\\n" \
            "$filename_display" \
            "$ct_m_f" "$dt_m_f" "$bpp_m_f" \
            "$ct_am_f" "$dt_am_f" "$bpp_am_f"
    done
    echo "\\bottomrule"
    echo "\\end{tabular}"
    echo "\\end{table}"
    echo ""

    # --- Table 3: Comparison of different configurations (Averages) ---
    echo "% --- Table 3: Comparison of different configurations (Averages) ---"
    echo "\\begin{table}[h!]"
    echo "\\centering"
    echo "\\caption{Comparison of different configurations}"
    echo "\\label{tab:compression_comparison_script}"
    echo "\\begin{tabular}{lrrrr}" # Removed one 'r' as there are 4 data columns + 1 label column
    echo "\\toprule"
    # Column headers: Baseline, Adaptive, Model, Adaptive+Model
    # Order from POSSIBLEFLAGS/FLAG_NAMES: Baseline (idx 0), Model (idx 1), Adaptive (idx 2), Adaptive+Model (idx 3)
    # Target table order: Baseline, Adaptive, Model, Adaptive+Model
    echo "                               & Baseline & Adaptive & Model  & \\text{Adaptive\\,+\\,Model} \\\\"
    echo "\\midrule"

    metrics=("Compression time (s)" "Decompression time (s)" "Bits per component (bpc)") # bpc as in example
    arrays_to_average=("STATS_COMPRESSTIME" "STATS_DECOMPRESSTIME" "STATS_EFFICIENCY")
    formats=("%.4f" "%.4f" "%.4f") # Example shows 4 decimal places for averages

    # Order of flags for Table 3 columns
    # Baseline (Original index 0)
    # Adaptive (Original index 2)
    # Model (Original index 1)
    # Adaptive+Model (Original index 3)
    # This array maps the desired column order to the original flag indices (0,1,2,3)
    table3_flag_indices=(0 2 1 3) 

    for k in "${!metrics[@]}"; do
        metric_name="${metrics[$k]}"
        array_name="${arrays_to_average[$k]}"
        format_string="${formats[$k]}"
        
        printf "%-30s" "$metric_name"
        
        for desired_idx in "${table3_flag_indices[@]}"; do
            # 'desired_idx' is the original index from POSSIBLEFLAGS (0 for Baseline, 1 for Model, etc.)
            # It's used to fetch data for the correct configuration
            
            SUM=0.0; COUNT=0
            for (( i=0; i<${#ALLFILES_COLLECT[@]}; i++ )); do
                stat_index=$((i * num_flags + desired_idx)) # desired_idx is j here essentially
                
                # Indirectly access the correct STATS array (STATS_COMPRESSTIME, STATS_DECOMPRESSTIME, or STATS_EFFICIENCY)
                eval "value=\${${array_name}[$stat_index]}"

                if [[ "$value" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
                    SUM=$(echo "$SUM + $value" | bc -l)
                    COUNT=$((COUNT + 1))
                fi
            done
            
            AVG="N/A"
            if [ $COUNT -gt 0 ]; then
                AVG=$(printf "$format_string" $(echo "$SUM / $COUNT" | bc -l))
            fi
            printf " & %s" "$AVG"
        done
        echo " \\\\"
    done
    echo "\\bottomrule"
    echo "\\end{tabular}"
    echo "\\end{table}"
fi

# --- Overall Averages Calculation and Print (Console only, not part of LaTeX as per example) ---
# This section can remain as is, or be removed if not needed for console output.
# For clarity, I'm leaving it as it was in the original script for console summary.
total_comp_time_sum=0.0
valid_comp_time_count=0
total_decomp_time_sum=0.0
valid_decomp_time_count=0
total_bpc_sum=0.0 # renamed from efficiency
valid_bpc_count=0

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

    bpc_val="${STATS_EFFICIENCY[$k]}" # using STATS_EFFICIENCY which stores bpp
    if [[ "$bpc_val" =~ ^[0-9]+(\.[0-9]+)?$ ]]; then
        total_bpc_sum=$(echo "$total_bpc_sum + $bpc_val" | bc -l)
        valid_bpc_count=$((valid_bpc_count + 1))
    fi
done

avg_comp_time_overall="N/A"
if [ "$valid_comp_time_count" -gt 0 ]; then
    avg_comp_time_overall=$(printf "%.4f" $(echo "$total_comp_time_sum / $valid_comp_time_count" | bc -l))
fi

avg_decomp_time_overall="N/A"
if [ "$valid_decomp_time_count" -gt 0 ]; then
    avg_decomp_time_overall=$(printf "%.4f" $(echo "$total_decomp_time_sum / $valid_decomp_time_count" | bc -l))
fi

avg_bpc_overall="N/A" #renamed
if [ "$valid_bpc_count" -gt 0 ]; then
    avg_bpc_overall=$(printf "%.4f" $(echo "$total_bpc_sum / $valid_bpc_count" | bc -l))
fi

echo ""
echo "------------------------------------------"
echo -e "${BLUE}Overall Averages Across All Configurations (Console Output):${NC}"
echo "Average Compression Time: $avg_comp_time_overall s"
echo "Average Decompression Time: $avg_decomp_time_overall s"
echo "Average bpp (bits per original byte): $avg_bpc_overall" # Changed label
echo "------------------------------------------"

rm -f compressed.tmp decompressed.tmp