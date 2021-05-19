package hycallparser.parameter;

public class ParameterNone extends Parameter {

    private static ParameterNone instance = new ParameterNone();

    public static ParameterNone instance() {
        return instance;
    }

    private ParameterNone() {}
}
