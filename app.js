'use strict';

// ── State ──────────────────────────────────────────────────────────────────
const STORAGE_KEY = 'short-videos-v1';

let videos = loadVideos();
let stream = null;
let mediaRecorder = null;
let chunks = [];
let recordedBlob = null;
let maxDuration = 15;
let timerInterval = null;
let elapsed = 0;
let currentVideoId = null;

// ── Persistence ─────────────────────────────────────────────────────────────
function loadVideos() {
  try {
    const raw = localStorage.getItem(STORAGE_KEY);
    return raw ? JSON.parse(raw) : [];
  } catch {
    return [];
  }
}

function saveVideos() {
  // Store only metadata; blob URLs can't be serialised across sessions.
  // For a demo we keep everything in memory and warn the user.
  localStorage.setItem(STORAGE_KEY, JSON.stringify(
    videos.map(v => ({ id: v.id, title: v.title, date: v.date, likes: v.likes, blobData: v.blobData }))
  ));
}

// ── DOM refs ────────────────────────────────────────────────────────────────
const feedBtn       = document.getElementById('feedBtn');
const recordBtn     = document.getElementById('recordBtn');
const tabFeed       = document.getElementById('tab-feed');
const tabRecord     = document.getElementById('tab-record');
const videoFeed     = document.getElementById('videoFeed');
const emptyMsg      = document.getElementById('emptyMsg');

const preview       = document.getElementById('preview');
const startBtn      = document.getElementById('startBtn');
const stopBtn       = document.getElementById('stopBtn');
const recBadge      = document.getElementById('recBadge');
const timerEl       = document.getElementById('timer');
const countdown     = document.getElementById('countdown');
const progressWrap  = document.querySelector('.progress-bar-wrap');

const review        = document.getElementById('review');
const reviewVideo   = document.getElementById('reviewVideo');
const titleInput    = document.getElementById('titleInput');
const saveBtn       = document.getElementById('saveBtn');
const discardBtn    = document.getElementById('discardBtn');

const modal         = document.getElementById('modal');
const modalBackdrop = document.getElementById('modalBackdrop');
const modalClose    = document.getElementById('modalClose');
const modalVideo    = document.getElementById('modalVideo');
const modalTitle    = document.getElementById('modalTitle');
const modalDate     = document.getElementById('modalDate');
const likeBtn       = document.getElementById('likeBtn');
const deleteBtn     = document.getElementById('deleteBtn');

// ── Tab switching ────────────────────────────────────────────────────────────
function switchTab(tab) {
  tabFeed.classList.toggle('active', tab === 'feed');
  tabRecord.classList.toggle('active', tab === 'record');
  feedBtn.classList.toggle('active', tab === 'feed');
  recordBtn.classList.toggle('active', tab === 'record');

  if (tab === 'feed') {
    stopCamera();
    renderFeed();
  }
}

feedBtn.addEventListener('click', () => switchTab('feed'));
recordBtn.addEventListener('click', () => switchTab('record'));

// ── Duration picker ──────────────────────────────────────────────────────────
document.querySelectorAll('.duration-btn').forEach(btn => {
  btn.addEventListener('click', () => {
    document.querySelectorAll('.duration-btn').forEach(b => b.classList.remove('active'));
    btn.classList.add('active');
    maxDuration = parseInt(btn.dataset.sec, 10);
  });
});

// ── Camera / recording ───────────────────────────────────────────────────────
startBtn.addEventListener('click', async () => {
  if (!stream) {
    await startCamera();
  } else {
    startRecording();
  }
});

stopBtn.addEventListener('click', () => stopRecording());

async function startCamera() {
  try {
    stream = await navigator.mediaDevices.getUserMedia({ video: { facingMode: 'user' }, audio: true });
    preview.srcObject = stream;
    startBtn.textContent = '録画開始';
  } catch (err) {
    alert('カメラへのアクセスが拒否されました。ブラウザの権限を確認してください。\n\n' + err.message);
  }
}

function stopCamera() {
  if (stream) {
    stream.getTracks().forEach(t => t.stop());
    stream = null;
  }
  preview.srcObject = null;
  startBtn.textContent = 'カメラを起動';
  stopBtn.classList.add('hidden');
  startBtn.classList.remove('hidden');
  recBadge.classList.remove('visible');
  timerEl.classList.remove('visible');
  clearInterval(timerInterval);
}

function startRecording() {
  chunks = [];
  const options = getSupportedMimeType();
  mediaRecorder = new MediaRecorder(stream, options);
  mediaRecorder.ondataavailable = e => { if (e.data.size > 0) chunks.push(e.data); };
  mediaRecorder.onstop = onRecordingStop;

  showCountdown(3, () => {
    mediaRecorder.start(100);
    startBtn.classList.add('hidden');
    stopBtn.classList.remove('hidden');
    recBadge.classList.add('visible');
    timerEl.classList.add('visible');
    elapsed = 0;
    renderTimer();
    timerInterval = setInterval(() => {
      elapsed++;
      renderTimer();
      if (elapsed >= maxDuration) stopRecording();
    }, 1000);
  });
}

function stopRecording() {
  if (mediaRecorder && mediaRecorder.state !== 'inactive') {
    mediaRecorder.stop();
  }
  clearInterval(timerInterval);
  recBadge.classList.remove('visible');
  timerEl.classList.remove('visible');
  stopBtn.classList.add('hidden');
  startBtn.classList.remove('hidden');
}

function onRecordingStop() {
  const mimeType = mediaRecorder.mimeType || 'video/webm';
  recordedBlob = new Blob(chunks, { type: mimeType });
  const url = URL.createObjectURL(recordedBlob);
  reviewVideo.src = url;
  review.classList.remove('hidden');
  titleInput.value = '';
  titleInput.focus();
}

function getSupportedMimeType() {
  const types = ['video/webm;codecs=vp9,opus', 'video/webm;codecs=vp8,opus', 'video/webm', 'video/mp4'];
  for (const type of types) {
    if (MediaRecorder.isTypeSupported(type)) return { mimeType: type };
  }
  return {};
}

// ── Countdown helper ─────────────────────────────────────────────────────────
function showCountdown(seconds, callback) {
  let n = seconds;
  countdown.style.opacity = '1';
  countdown.textContent = n;
  const iv = setInterval(() => {
    n--;
    if (n <= 0) {
      clearInterval(iv);
      countdown.style.opacity = '0';
      countdown.textContent = '';
      callback();
    } else {
      countdown.textContent = n;
    }
  }, 1000);
}

function renderTimer() {
  const m = String(Math.floor(elapsed / 60)).padStart(2, '0');
  const s = String(elapsed % 60).padStart(2, '0');
  timerEl.textContent = `${m}:${s}`;
}

// ── Save / discard ────────────────────────────────────────────────────────────
saveBtn.addEventListener('click', async () => {
  if (!recordedBlob) return;

  const title = titleInput.value.trim() || `ショート動画 ${videos.length + 1}`;

  // Convert blob to base64 so it survives page reloads
  const blobData = await blobToBase64(recordedBlob);

  const video = {
    id: Date.now().toString(),
    title,
    date: new Date().toLocaleString('ja-JP'),
    likes: 0,
    liked: false,
    blobData,
    mimeType: recordedBlob.type,
  };

  videos.unshift(video);
  saveVideos();

  review.classList.add('hidden');
  recordedBlob = null;
  URL.revokeObjectURL(reviewVideo.src);
  reviewVideo.src = '';

  switchTab('feed');
});

discardBtn.addEventListener('click', () => {
  review.classList.add('hidden');
  recordedBlob = null;
  URL.revokeObjectURL(reviewVideo.src);
  reviewVideo.src = '';
});

// ── Base64 helpers ────────────────────────────────────────────────────────────
function blobToBase64(blob) {
  return new Promise((resolve, reject) => {
    const reader = new FileReader();
    reader.onload = () => resolve(reader.result);
    reader.onerror = reject;
    reader.readAsDataURL(blob);
  });
}

// ── Feed rendering ────────────────────────────────────────────────────────────
function renderFeed() {
  videoFeed.innerHTML = '';

  if (videos.length === 0) {
    emptyMsg.classList.add('visible');
    return;
  }
  emptyMsg.classList.remove('visible');

  videos.forEach(v => {
    const card = document.createElement('div');
    card.className = 'video-card';
    card.innerHTML = `
      <video src="${v.blobData}" preload="metadata" muted playsinline></video>
      <div class="play-icon"></div>
      <div class="video-card-overlay">
        <p class="video-card-title">${escapeHtml(v.title)}</p>
        <p class="video-card-meta">${v.date}</p>
      </div>
      <span class="video-card-likes">${v.likes > 0 ? `♥ ${v.likes}` : ''}</span>
    `;
    card.addEventListener('click', () => openModal(v.id));
    videoFeed.appendChild(card);
  });
}

function escapeHtml(str) {
  return str.replace(/[&<>"']/g, c => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
}

// ── Modal ─────────────────────────────────────────────────────────────────────
function openModal(id) {
  currentVideoId = id;
  const v = videos.find(x => x.id === id);
  if (!v) return;

  modalVideo.src = v.blobData;
  modalTitle.textContent = v.title;
  modalDate.textContent = v.date;
  likeBtn.textContent = `${v.liked ? '♥' : '♡'} いいね ${v.likes > 0 ? v.likes : ''}`.trim();
  likeBtn.classList.toggle('liked', v.liked);

  modal.classList.remove('hidden');
  modalVideo.play().catch(() => {});
}

function closeModal() {
  modal.classList.add('hidden');
  modalVideo.pause();
  modalVideo.src = '';
  currentVideoId = null;
}

modalClose.addEventListener('click', closeModal);
modalBackdrop.addEventListener('click', closeModal);

likeBtn.addEventListener('click', () => {
  const v = videos.find(x => x.id === currentVideoId);
  if (!v) return;
  v.liked = !v.liked;
  v.likes += v.liked ? 1 : -1;
  likeBtn.textContent = `${v.liked ? '♥' : '♡'} いいね ${v.likes > 0 ? v.likes : ''}`.trim();
  likeBtn.classList.toggle('liked', v.liked);
  saveVideos();
  renderFeed();
});

deleteBtn.addEventListener('click', () => {
  if (!currentVideoId) return;
  if (!confirm('この動画を削除しますか？')) return;
  videos = videos.filter(x => x.id !== currentVideoId);
  saveVideos();
  closeModal();
  renderFeed();
});

// ── Init ──────────────────────────────────────────────────────────────────────
renderFeed();
switchTab('feed');
feedBtn.classList.add('active');
