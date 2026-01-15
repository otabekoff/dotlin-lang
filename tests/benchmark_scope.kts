fun currentTimeMillis(): Long = System.currentTimeMillis()

fun benchmark() {
    val a = "global"
    run {
        val b = "first"
        run {
            val c = "second"
            run {
                val d = "third"
                run {
                    val e = "fourth"
                    run {
                        val f = "fifth"

                        val start = currentTimeMillis()
                        var i = 0
                        while (i < 1_000_000) {
                            val x = a
                            val y = b
                            val z = e
                            i += 1
                        }
                        val end = currentTimeMillis()
                        println("Time taken: ${end - start}ms")
                    }
                }
            }
        }
    }
}

benchmark()
