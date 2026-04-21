import argparse
import random
import sys
from datetime import datetime
from pathlib import Path

HOOKS = [
    "40代でドラムを始めた。「遅すぎる」なんて嘘だった。",
    "「もう遅い」そう思って、何年も後回しにしてきた。",
    "50代でスティックを初めて握った日、何かが変わった。",
    "「{theme}」40代の私が、ようやく気づいたこと。",
    "ドラムを始めるのに、年齢は一切関係なかった。マジで。",
    "「自分には無理」と思い込んでた。全部、間違いだった。",
]

EMPATHIES = [
    "「今さら始めても」という言葉が、ずっと自分の邪魔をしていた。",
    "若い人たちに囲まれるのが怖くて、なかなか踏み出せなかった。",
    "仕事と家族でいっぱいの毎日に、自分の番が来ない気がしてた。",
    "体がついてくるか不安で、ずっと一歩が踏み出せなかった。",
    "下手だったら恥ずかしいと、ずっと自分にブレーキをかけてた。",
]

DISCOVERIES = [
    "でも実際に叩いてみたら、年齢なんて全然関係なかった。",
    "40代の耳は、リズムをとっくに知っていた。",
    "初めて叩いた瞬間、「ああ、これだ」とはっきりわかった。",
    "積み重ねてきた人生経験が、全部リズムになる感覚があった。",
    "下手でいい場所が、ここにはちゃんとあった。",
]

INSIGHTS = [
    "うまくなるより続けること。それが全てだとわかった。",
    "ドラムを叩くと、今日の疲れが音になって消えていく。",
    "40代だからこそ、音の奥にある深みが感じられる。",
    "音を出すことが、忘れていた自分を呼び戻してくれた。",
    "下手でいい。この「楽しい」が、全部の答えだった。",
]

AFTERGLOWS = [
    "遅すぎることなんて、なかった。",
    "あなたも、まず1打だけ叩いてみて。",
    "音を出した瞬間から、もう始まっている。",
    "今日が、これからで一番若い日だ。",
    "スティックが、第二の人生の扉になった。",
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
        description="X投稿下書き生成ツール（ターゲット：40〜50代ドラム初心者）"
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
