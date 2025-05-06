// Add this script before any others to trace requestIdleCallback usage

(function() {
    // First create polyfill
    if (!window.requestIdleCallback) {
      window.requestIdleCallback = function(callback) {
        console.log("Polyfill requestIdleCallback called");
        console.trace("Call stack for requestIdleCallback");
        
        const start = Date.now();
        return setTimeout(function() {
          callback({
            didTimeout: false,
            timeRemaining: function() {
              return Math.max(0, 50 - (Date.now() - start));
            }
          });
        }, 1);
      };
    } else {
      // If browser has native support, wrap it to trace calls
      const originalRequestIdleCallback = window.requestIdleCallback;
      window.requestIdleCallback = function(callback, options) {
        console.log("Native requestIdleCallback called");
        console.trace("Call stack for requestIdleCallback");
        return originalRequestIdleCallback(callback, options);
      };
    }
    
    if (!window.cancelIdleCallback) {
      window.cancelIdleCallback = function(id) {
        clearTimeout(id);
      };
    }
    
    // Monitor script loading
    const originalCreateElement = document.createElement;
    document.createElement = function() {
      const element = originalCreateElement.apply(this, arguments);
      if (arguments[0].toLowerCase() === 'script') {
        element.addEventListener('load', function() {
          console.log("Script loaded:", element.src || "inline script");
        });
      }
      return element;
    };
    
    console.log("Debug tracing initialized");
  })();