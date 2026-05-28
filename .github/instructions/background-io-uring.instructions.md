# Background Knowledge — io_uring

## 왜 io_uring인가

기존 방식 (epoll + read/write):
- 매 I/O마다 syscall → 커널 진입/복귀 = context switch 비용
- 초당 수십만 요청에서 병목

io_uring:
- 커널과 유저스페이스가 **공유 메모리 큐**로 통신
- syscall 최소화, 배치 처리 가능

## 핵심 구조

```
┌─────────────────────────────────────┐
│          Shared Memory              │
│                                     │
│  [SQ] Submission Queue → 할 일     │
│  유저가 요청 추가 (syscall 없음)    │
│                                     │
│  [CQ] Completion Queue → 결과      │
│  커널이 결과 추가 (syscall 없음)    │
└─────────────────────────────────────┘
```

- **SQE** (Submission Queue Entry): 요청 1개 (opcode, fd, buf, len, user_data)
- **CQE** (Completion Queue Entry): 결과 1개 (user_data, res)
- **submit()**: SQ에 쌓인 요청을 커널에 제출 (유일한 syscall)

## 동작 흐름 (echo server)

```
1. io_uring_queue_init(256, &ring, 0)  // 큐 깊이 256
2. prep_accept(server_fd)              // SQ에 accept 추가
3. io_uring_submit(&ring)              // 제출
4. io_uring_wait_cqe(&ring, &cqe)     // CQ에서 결과 대기
5. client_fd = cqe->res                // 새 연결
6. prep_read(client_fd, buf)           // read 요청 추가
7. submit → wait → prep_write → ...   // 반복
```

## liburing API 요약

```cpp
// 초기화/정리
io_uring_queue_init(depth, &ring, flags);
io_uring_queue_exit(&ring);

// SQE 가져오기 + 준비
struct io_uring_sqe* sqe = io_uring_get_sqe(&ring);
io_uring_prep_accept(sqe, fd, addr, addrlen, flags);
io_uring_prep_read(sqe, fd, buf, len, offset);
io_uring_prep_write(sqe, fd, buf, len, offset);
sqe->user_data = my_id;  // CQE에서 식별용

// 제출
io_uring_submit(&ring);

// 결과 수거
io_uring_wait_cqe(&ring, &cqe);      // blocking
io_uring_peek_cqe(&ring, &cqe);      // non-blocking
io_uring_cqe_seen(&ring, cqe);       // "확인했음" 마킹
```

## epoll vs io_uring

| | epoll | io_uring |
|--|-------|----------|
| 알림 방식 | "준비됐다" (readiness) | "다 했다" (completion) |
| syscall 횟수 | 많음 (wait + read + write) | 적음 (submit 1번에 N개) |
| 배치 | ❌ | ✅ |
| zero-copy | ❌ | ✅ (fixed buffer) |

## Lumina에서의 역할

IoRing RAII 클래스로 감싸서:
```cpp
namespace lumina {
class IoRing {
    IoRing(unsigned queue_depth);   // io_uring_queue_init
    ~IoRing();                      // io_uring_queue_exit

    void prep_accept(int server_fd);
    void prep_read(int fd, std::span<char> buf);
    void prep_write(int fd, std::span<const char> buf);
    void submit();
    CQE wait_completion();
};
}
```

## 참고 자료
- Lord of the io_uring: https://unixism.net/liouring/what-is-io_uring/
- liburing examples: https://github.com/axboe/liburing/tree/master/examples
- io_uring man page: https://man7.org/linux/man-pages/man7/io_uring.7.html
