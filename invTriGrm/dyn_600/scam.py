import csv

replacements = {
    0:   ("justicw", "justice", "Adjacent Horizontal Slips", "NORMAL", "7", "typed w for e (same row)"),
    242: ("fosh", "fish", "Adjacent Horizontal Slips", "HARD", "4", "typed o for i (same row)"),
    245: ("ladt", "last", "Adjacent Horizontal Slips", "HARD", "4", "typed d for s (same row)"),
    246: ("cst", "cat", "Adjacent Horizontal Slips", "HARD", "3", "typed s for a (same row)"),
    252: ("wopd", "wood", "Adjacent Horizontal Slips", "HARD", "4", "typed p for o (same row)"),
    257: ("srake", "stake", "Adjacent Horizontal Slips", "HARD", "5", "typed r for t (same row)"),
    267: ("pirt", "port", "Adjacent Horizontal Slips", "HARD", "4", "typed i for o (same row)"),
    269: ("chsnce", "chance", "Adjacent Horizontal Slips", "HARD", "6", "typed s for a (same row)"),
    272: ("hsnd", "hand", "Adjacent Horizontal Slips", "HARD", "4", "typed s for a (same row)"),
    282: ("brsnd", "brand", "Adjacent Horizontal Slips", "HARD", "5", "typed s for a (same row)"),
    290: ("fsll", "fall", "Adjacent Horizontal Slips", "HARD", "4", "typed s for a (same row)"),
    297: ("orher", "other", "Adjacent Horizontal Slips", "HARD", "5", "typed r for t (same row)"),
    306: ("gome", "home", "Adjacent Horizontal Slips", "HARD", "4", "typed g for h (same row)"),
    309: ("cljmb", "climb", "Vertical/Diagonal Slips", "HARD", "5", "typed j for i (row below)"),
    314: ("wifh", "wish", "Adjacent Horizontal Slips", "HARD", "4", "typed f for s (same row)"),
    315: ("stusy", "study", "Adjacent Horizontal Slips", "HARD", "5", "typed s for d (same row)"),
    340: ("fsmily", "family", "Adjacent Horizontal Slips", "HARD", "6", "typed s for a (same row)"),
    344: ("candp", "camp", "Adjacent Horizontal Slips", "HARD", "4", "typed d for m (row above)"),
    348: ("drsk", "desk", "Adjacent Horizontal Slips", "HARD", "4", "typed r for e (same row)"),
    483: ("hsot", "host", "Structural & Phonetic", "HARD", "4", "swapped so"),
    486: ("prak", "park", "Structural & Phonetic", "HARD", "4", "swapped ar"),
    488: ("fsat", "fast", "Structural & Phonetic", "HARD", "4", "swapped as"),
    489: ("cricuit", "circuit", "Structural & Phonetic", "HARD", "7", "swapped ir"),
    499: ("brigde", "bridge", "Structural & Phonetic", "HARD", "6", "swapped dg"),
    504: ("chere", "cheer", "Structural & Phonetic", "HARD", "5", "typed e for r (same row)"),
    506: ("dostance", "distance", "Adjacent Horizontal Slips", "HARD", "8", "typed o for i (same row)"),
    510: ("positoon", "position", "Adjacent Horizontal Slips", "HARD", "8", "typed o for i (same row)"),
    511: ("syn", "sun", "Adjacent Horizontal Slips", "HARD", "3", "typed y for u (same row)"),
    514: ("mowt", "most", "Adjacent Horizontal Slips", "HARD", "4", "typed w for s (row above)"),
    545: ("wird", "word", "Adjacent Horizontal Slips", "HARD", "4", "typed i for o (same row)"),
}

input_file = "test_cases_600.csv"
rows = []

with open(input_file, mode="r", newline="", encoding="utf-8") as f:
    reader = csv.reader(f)
    header = [h.strip() for h in next(reader)]
    
    for row in reader:
        if not row:
            continue
        row_id = int(row[0].strip())
        if row_id in replacements:
            q, gt, cat, ty, tlen, note = replacements[row_id]
            rows.append([str(row_id), q, gt, cat, ty, str(tlen), note])
        else:
            rows.append([c.strip() for c in row])

with open(input_file, mode="w", newline="", encoding="utf-8") as f:
    writer = csv.writer(f)
    writer.writerow(["id", "query", "ground_truth", "category", "type", "truth_len", "notes"])
    writer.writerows(rows)

print("Updated test_cases_600.csv successfully.")