import time

def currentTimeMillis():
    return int(time.time() * 1000)

def benchmark():
    a = "global"
    # keep the same “nested scopes” vibe using nested functions
    def scope1():
        b = "first"
        def scope2():
            c = "second"
            def scope3():
                d = "third"
                def scope4():
                    e = "fourth"
                    def scope5():
                        f = "fifth"

                        start = currentTimeMillis()
                        i = 0
                        while i < 1_000_000:
                            x = a
                            y = b
                            z = e
                            i += 1
                        end = currentTimeMillis()
                        print("Time taken:", (end - start), "ms")
                    scope5()
                scope4()
            scope3()
        scope2()
    scope1()

benchmark()
