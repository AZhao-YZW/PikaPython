class Response:
    content_length: int
    text: str
    state_code: int
    headers: str
    url: str
    session_address: int

    def json(self) -> dict: ...

    def request(self, method: str, **kwargs) -> int: ...
    def request_init(self, method: str) -> int: ...
    def request_del(self, ) -> None: ...
    def proto_write(self, proto: str = None) -> int: ...
    def urlencode_write(self, s1: str, s2: str = None, start: str = None, connect: str = None) -> int: ...
    def header_write(self, header: str, value: str) -> int: ...

    def __init__(): ...
    def __del__(): ...
