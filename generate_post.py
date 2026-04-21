import argparse
import random
import sys
from datetime import datetime
from pathlib import Path

# Voice: 40年近いドラム歴・講師歴20年超のドラマーが
#        40〜50代の初心者に語りかける投稿

HOOKS = [
    "15歳から叩いて、気づけば40年。今も確信があること。",
    "講師として20年、何百人の大人を教えてきた私が言う。",
    "40代からドラムを始めた生徒を、何百人も見てきた。",
    "「{theme}」ドラム講師20年の私が、正直に答えます。",
    "20年教え続けてきた。今も絶対に変わらない確信がある。",
    "ドラム歴40年の私が、今も一番感動する瞬間がある。",
]

EMPATHIES = [
    "「もう遅い」そう言って教室に来た生徒が、何人いただろう。",
    "「私なんかが…」って小さくなって来る方を、何人も見てきた。",
    "年齢を気にして最初から謝る人、本当に多い。知ってほしい。",
    "始める前から「体がついていくか」と不安そうな顔をしてた。",
    "「今さら」と恥ずかしそうに言う人ほど、実は伸びる。",
]

DISCOVERIES = [
    "でも半年後、みんな同じ最高の顔をして叩いている。",
    "40代の耳は正直すごい。人生でリズムをもう積んできている。",
    "大人の生徒の集中力は、若い子の比じゃない。本当に。",
    "人生経験があるほど、音に深みが出る。これは本当のこと。",
    "初めて自分のリズムが出た瞬間の顔を、何百回見てきたか。",
]

INSIGHTS = [
    "年齢は関係ない。40年叩き続けてきた私が、断言します。",
    "遅く始めた人ほど、一打一打を大切にする。それが強みだ。",
    "「楽しい」という感性は、年齢では絶対に衰えない。",
    "40代から始めて輝いた生徒を、私はたくさん知っている。",
    "音楽に出会うのに、遅すぎることは絶対にない。これが真実。",
]

AFTERGLOWS = [
    "あなたの音を、ぜひ聴かせてください。",
    "今が、始めるちょうどいい時です。",
    "一歩踏み出せば、私がちゃんとサポートします。",
    "その一打を、一緒に鳴らしましょう。",
    "叩きたいなら、もう迷わなくていい。",
]


def pick(templates: list[str], theme: str) -> str:
    return random.choice(templates).format(theme=theme)


def generate_three_posts(theme: str) -> list[dict]:
    sampled_hooks = random.sample(HOOKS, min(3, len(HOOKS)))
    posts = []
    for hook_template in sampled_hooks:
        posts.append({
            "hook":      hook_template.format(theme=theme),
            "empathy":   pick(EMPATHIES, theme),
            "discovery": pick(DISCOVERIES, theme),
            "insight":   pick(INSIGHTS, theme),
            "afterglow": pick(AFTERGLOWS, theme),
        })
    return posts


def format_post(parts: dict) -> str:
    return "\n\n".join([
        parts["hook"],
        parts["empathy"],
        parts["discovery"],
        parts["insight"],
        parts["afterglow"],
    ])


def save_output(theme: str, posts: list[str]) -> str:
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    filename = f"draft_{timestamp}.txt"

    now_str = datetime.now().strftime("%Y-%m-%d %H:%M:%S")
    sep = "=" * 50
    div = "-" * 50

    lines = [
        sep,
        "X 投稿下書き（3パターン）",
        sep,
        f"生成日時 : {now_str}",
        f"テーマ   : {theme}",
    ]

    for i, post_text in enumerate(posts, 1):
        char_count = len(post_text)
        lines += [
            "",
            div,
            f"【パターン {i}】 {char_count}文字",
            div,
            "",
            post_text,
        ]

    lines += ["", sep, ""]
    Path(filename).write_text("\n".join(lines), encoding="utf-8")
    return filename


def main():
    parser = argparse.ArgumentParser(
        description="X投稿下書き生成ツール（ドラム講師・ベテランドラマー視点）"
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

    posts_data = generate_three_posts(theme)
    post_texts = [format_post(p) for p in posts_data]

    div = "─" * 40
    for i, text in enumerate(post_texts, 1):
        char_count = len(text)
        print(f"\n{div}")
        print(f"【パターン {i}】 {char_count}文字")
        print(f"{div}\n")
        print(text)

    print()
    filename = save_output(theme, post_texts)
    print(f"保存しました: {filename}")


if __name__ == "__main__":
    main()
