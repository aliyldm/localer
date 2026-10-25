const { contextBridge, ipcRenderer } = require('electron');
contextBridge.exposeInMainWorld('localer', {
  info: () => ipcRenderer.invoke('app:info'),
  create: data => ipcRenderer.invoke('room:create', data),
  join: data => ipcRenderer.invoke('room:join', data),
  leave: () => ipcRenderer.invoke('room:leave'),
  copy: text => ipcRenderer.invoke('app:copy', text),
  elevate: () => ipcRenderer.invoke('app:elevate'),
  onState: callback => {
    const listener = (_event, state) => callback(state);
    ipcRenderer.on('engine:state', listener);
    return () => ipcRenderer.removeListener('engine:state', listener);
  }
});
