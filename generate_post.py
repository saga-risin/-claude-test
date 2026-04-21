import argparse
import random
import sys
from datetime import datetime
from pathlib import Path

HOOKS = [
    "「{theme}って、実は仕事にも効くって知ってた？」",
    "「{theme}について、誰も教えてくれなかったこと。」",
    "「{theme} ─ これを知ったとき、残業後の過ごし方が変わった。」",
    "「{theme}、始めるのに遅すぎるなんてことはない。」",
]

EMPATHIES = [
    "毎日コードと向き合って、頭が煮えてくる感覚、あるよな。",
    "趣味を始めようとして、何から手をつければいいかわからない週末。",
    "仕事で論理ばかり使っていると、感覚的なことが苦手になってくる。",
    "社会人になってから、「やりたいこと」を後回しにしすぎている気がする。",
]

DISCOVERIES = [
    "でも {theme} って、難しさよりも「ノリ」が先に来るんだよね。",
    "{theme} を調べてみたら、初心者でも2週間で形になると知った。",
    "実は {theme}、ストレス発散に最適だという研究がある。",
    "{theme} って、始める前の壁が一番高いだけで、入ったら意外と沼。",
]

INSIGHTS = [
    "リズムを刻むと、頭のノイズがスッと消える感覚がある。",
    "趣味って、うまくなることより続けることが一番の成果だと思う。",
    "ドラムは全身運動。体を動かすと、次の日の仕事への集中力が変わる。",
    "「できた」の1回目が、すべてのモチベーションになる。",
]

AFTERGLOWS = [
    "今夜、スティックを握ってみようか。",
    "あなたはどんな趣味で、自分を取り戻していますか？",
    "叩く音が、自分だけのリズムになる。",
    "仕事以外の自分を、もう一度育ててみよう。",
]


def pick(templates: list[str], theme: str) -> str:
    return random.choice(templates).format(theme=theme)


def generate_post(theme: str) -> dict:
    return {
        "hook":      pick(HOOKS, theme),
        "empathy":   pick(EMPATHIES, theme),
        "discovery": pick(DISCOVERIES, theme),
        "insight":   pick(INSIGHTS, theme),
        "afterglow": pick(AFTERGLOWS, theme),
    }


def format_post(parts: dict) -> str:
    return "\n\n".join([
        parts["hook"],
        parts["empathy"],
        parts["discovery"],
        parts["insight"],
        parts["afterglow"],
    ])


def save_output(theme: str, post_text: str) -> str:
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"draft_{timestamp}.txt"
    path = Path(filename)

    char_count = len(post_text)
    now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")

    sep = "=" * 50
    div = "-" * 50
    content = (
        f"{sep}\n"
        f"X 投稿下書き\n"
        f"{sep}\n"
        f"生成日時 : {now_str}\n"
        f"テーマ   : {theme}\n\n"
        f"{div}\n\n"
        f"{post_text}\n\n"
        f"{div}\n"
        f"文字数: {char_count}文字\n"
        f"{sep}\n"
    )

    path.write_text(content, encoding="utf-8")
    return filename


def main():
    parser = argparse.ArgumentParser(
        description="X投稿下書き生成ツール（ターゲット：26歳ITサラリーマン × ドラム趣味）"
    )
    parser.add_argument("theme", nargs="?", help="投稿のテーマ")
    args = parser.parse_args()

    theme = args.theme

    if not theme:
        theme = input("テーマを入力してください: ").strip()

    if not theme:
        sys.exit("エラー: テーマが空です。テーマを入力してください。")

    if len(theme) > 100:
        sys.exit(f"エラー: テーマが長すぎます（{len(theme)}文字）。100文字以内で入力してください。")

    parts = generate_post(theme)
    post_text = format_post(parts)

    print()
    print(post_text)
    print()

    filename = save_output(theme, post_text)
    print(f"保存しました: {filename}")


if __name__ == "__main__":
    main()
