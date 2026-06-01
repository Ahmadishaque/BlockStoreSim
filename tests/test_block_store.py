import pathlib
import subprocess


ROOT = pathlib.Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build-test"
BINARY = BUILD_DIR / "blockstoresim"


def build_binary() -> pathlib.Path:
    BUILD_DIR.mkdir(exist_ok=True)
    subprocess.run(["cmake", "-S", str(ROOT), "-B", str(BUILD_DIR)], check=True)
    subprocess.run(["cmake", "--build", str(BUILD_DIR)], check=True)
    return BINARY


def run_cli(commands: str) -> str:
    binary = build_binary()
    completed = subprocess.run(
        [str(binary)],
        input=commands,
        text=True,
        capture_output=True,
        check=True,
    )
    return completed.stdout


def parse_stats(output: str) -> dict[str, int]:
    stats = {}
    for line in output.splitlines():
        if "=" not in line:
            continue
        key, value = line.split("=", 1)
        if value.isdigit():
            stats[key] = int(value)
    return stats


def test_deduplicates_identical_payloads():
    output = run_cli(
        "\n".join(
            [
                "WRITE user_1 hello",
                "WRITE user_2 hello",
                "STATS",
                "EXIT",
            ]
        )
    )

    stats = parse_stats(output)
    assert stats["logical_records"] == 2
    assert stats["physical_blocks"] == 1
    assert stats["duplicate_writes"] == 1
    assert stats["logical_bytes"] == 10
    assert stats["physical_bytes"] == 5


def test_overwrite_updates_reference_counts():
    output = run_cli(
        "\n".join(
            [
                "WRITE user_1 hello",
                "WRITE user_2 hello",
                "WRITE user_1 new_payload",
                "STATS",
                "READ user_1",
                "READ user_2",
                "EXIT",
            ]
        )
    )

    stats = parse_stats(output)
    assert stats["logical_records"] == 2
    assert stats["physical_blocks"] == 2
    assert stats["overwrite_writes"] == 1
    assert "VALUE key=user_1 payload=new_payload" in output
    assert "VALUE key=user_2 payload=hello" in output


def test_delete_marks_block_reclaimable_then_compacts():
    output = run_cli(
        "\n".join(
            [
                "WRITE user_1 only_payload",
                "DELETE user_1",
                "STATS",
                "COMPACT",
                "STATS",
                "EXIT",
            ]
        )
    )

    first_stats_start = output.index("logical_records=0")
    first_stats_text = output[first_stats_start:]
    assert "reclaimable_blocks=1" in first_stats_text
    assert "OK compact removed_blocks=1" in output
    assert output.strip().endswith("reclaimable_bytes=0")


def test_snapshot_is_deterministic_and_contains_hash_metadata():
    output = run_cli(
        "\n".join(
            [
                "WRITE beta payload_b",
                "WRITE alpha payload_a",
                "SNAPSHOT",
                "EXIT",
            ]
        )
    )

    snapshot_lines = [line for line in output.splitlines() if " hash=" in line]
    assert snapshot_lines[0].startswith("alpha hash=")
    assert snapshot_lines[1].startswith("beta hash=")
    assert "size=9" in snapshot_lines[0]
    assert "size=9" in snapshot_lines[1]
