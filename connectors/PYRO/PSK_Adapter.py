
import sslpsk
import Pyro
from Pyro.protocol import _connect_socket,TCPConnection,PYROAdapter
from Pyro.errors import ConnectionDeniedError, ProtocolError
from Pyro.util import Log

#
# The TLS-PSK adapter that handles SSL connections instead of regular sockets,
# but using Pre Shared Keys instead of Certificates
#
class PYROPSKAdapter(PYROAdapter):
    # This is essentialy the same as in Pyro/protocol.py
    # only raw_sock wrapping into sock through sslpsk.wrap_socket was added
    # Pyro unfortunately doesn't allow cleaner customization
    def bindToURI(self,URI):
        with self.lock:   # only 1 thread at a time can bind the URI
            try:
                self.URI=URI

                # This are the statements that differ from Pyro/protocol.py
                raw_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                _connect_socket(raw_sock, URI.address, URI.port, self.timeout)
                sock = sslpsk.wrap_socket(
                    raw_sock, psk=Pyro.config.PYROPSK, server_side=False)
                # all the rest is the same as in Pyro/protocol.py 

                conn=TCPConnection(sock, sock.getpeername())
                # receive the authentication challenge string, and use that to build the actual identification string.
                try:
                    authChallenge=self.recvAuthChallenge(conn)
                except ProtocolError,x:
                    # check if we were denied
                    if hasattr(x,"partialMsg") and x.partialMsg[:len(self.denyMSG)]==self.denyMSG:
                        raise ConnectionDeniedError(Pyro.constants.deniedReasons[int(x.partialMsg[-1])])
                    else:
                        raise
                # reply with our ident token, generated from the ident passphrase and the challenge
                msg = self._sendConnect(sock,self.newConnValidator.createAuthToken(self.ident, authChallenge, conn.addr, self.URI, None) )
                if msg==self.acceptMSG:
                    self.conn=conn
                    self.conn.connected=1
                    Log.msg('PYROAdapter','connected to',str(URI))
                    if URI.protocol=='PYROLOC':
                        self.resolvePYROLOC_URI("PYRO") # updates self.URI
                elif msg[:len(self.denyMSG)]==self.denyMSG:
                    try:
                        raise ConnectionDeniedError(Pyro.constants.deniedReasons[int(msg[-1])])
                    except (KeyError,ValueError):
                        raise ConnectionDeniedError('invalid response')
            except socket.error:
                Log.msg('PYROAdapter','connection failed to URI',str(URI))
                raise ProtocolError('connection failed')

_getProtocolAdapter = Pyro.protocol.getProtocolAdapter
def getProtocolAdapter(protocol):
    if protocol in ('PYROPSK', 'PYROLOCPSK'):
        return PYROPSKAdapter()
    _getProtocolAdapter(protocol)

Pyro.protocol.getProtocolAdapter = getProtocolAdapter

