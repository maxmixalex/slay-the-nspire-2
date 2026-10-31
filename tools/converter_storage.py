"""Reproducible lossless UI15-to-NSP2/44 storage stages for the converter."""
from pathlib import Path

CONCERNS = ("interface", "cards", "characters", "enemies", "bosses", "rooms", "effects", "shared")


def edition_files(directory, codes):
    directory = Path(directory)
    return [directory / "nSpire2.dat.tns",
            *(directory / ("nSpire2." + name + ".dat.tns") for name in CONCERNS),
            *(directory / ("nSpire2." + code + ".dat.tns") for code in codes)]


def add_storage_stages(recipe, source_editions, codes):
    """Return the verified logical-English 44 path and its matching editions.

    These stages start with newly generated UI15 resources. They do not use a
    previous release, a historical migration receipt, or a private raw spool.
    """
    codes = list(codes)
    if "eng" not in codes or len(codes) != len(set(codes)):
        raise ValueError("Storage stages require English and unique locale codes")
    current = recipe.directory("storage40")
    recipe.add("storage40", "storage_parts",
               ["--input", source_editions, "--output", current],
               edition_files(current, codes) + [current / "storage-receipt.json"])
    for name, module, receipt in (
        ("index41", "predict_index_resources", "index-prediction-receipt.json"),
        ("coalesced41", "coalesce_tile_resources", "coalesced-tiles-receipt.json"),
        ("compact42", "compact_tile_slots", "compact-slots-receipt.json"),
        ("planes43", "compact_retained_planes", "retained-planes-receipt.json"),
        ("paired44", "merge_tile_groups", "paired-tiles-receipt.json"),
    ):
        output = recipe.directory(name)
        work = recipe.directory("cache") / name
        arguments = ["--input", current, "--output", output, "--work", work,
                     "--workers", str(min(2, recipe.workers))]
        if module != "predict_index_resources":
            arguments.extend(["--locales", *codes])
        outputs = edition_files(output, codes) + [output / receipt]
        if name == "paired44":
            outputs.append(work / "eng-paired44.nsp")
        recipe.add(name, module, arguments, outputs)
        current = output
    return recipe.directory("cache") / "paired44" / "eng-paired44.nsp", current
