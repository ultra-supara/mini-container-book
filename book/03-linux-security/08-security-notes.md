# システムをセキュアにするための留意点

### 1. 可能ならroot特権を使わない

rootは何でも出来るので便利な一方危険性も大きいため、アクセスしたいリソースのパーミッションを設定することで一般ユーザーで実現することを考えます.

### 2. root特権を持っておかない

root特権を使う場合でも、不要な時にはなるべく早くeuidをrootではないものに設定します。今後rootが必要になることがないのであれば、suidやruidもrootでないものにすることで恒久的にrootを捨てます.

**権限を正しく設定して堅牢なシステムを作ろう！！**

- <https://i-love.sakura.ne.jp/tomoyo/spc2011-kumaneko.pdf>
- <https://i-love.sakura.ne.jp/tomoyo/CaitSith-ja.pdf>

---
